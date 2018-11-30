#include <map>

#include "HaiKangApplication.h"
#include "HaiKangChannel.h"
#include "PlayM4.h"
#include "Config.h"
#include "wqueue.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h> 
#include <libavformat/avformat.h>
#ifdef __cplusplus 
}
#endif

const std::string DVR_HOST = "dvr.host";
const std::string DVR_PORT = "dvr.port";
const std::string DVR_USERNAME = "dvr.username";
const std::string DVR_PASSWORD = "dvr.password";

const std::string logDir = "log";

HaiKangApplication::HaiKangApplication(): Application("HaiKangApplication") {}

struct ChannelInfo {
    LONG port;
    LONG handler;
    int channel;
};

std::map<LONG, ChannelInfo*> channelMap;

wqueue<std::pair<int, cv::Mat>> imgQueue;

std::map<int, cv::Mat> results;


int getIPConInfo(LPNET_DVR_IPPARACFG pIPConInfo, LONG userId)
{
   if(userId < 0 && pIPConInfo ==NULL)
   {
      return HPR_ERROR;
   }

   int iRet;
   DWORD uiReturnLen;
   iRet = NET_DVR_GetDVRConfig(userId, NET_DVR_GET_IPPARACFG, 0, pIPConInfo, sizeof(NET_DVR_IPPARACFG), &uiReturnLen);
    if(0 == iRet)
    {
       iRet = NET_DVR_GetLastError();
       printf("NET_DVR_Login_V30 error: %d\n", iRet);
       return HPR_ERROR;
    }

    return HPR_OK;
}

void CALLBACK decodeFunc(LONG nPort, 
                            char * pBuf, 
                            int nSize, 
                            FRAME_INFO * pFrameInfo, 
                            void* nReserved1, 
                            int nReserved2) {
    cv::Mat yv12 = cv::Mat(pFrameInfo->nHeight * 3 / 2 , 
                            pFrameInfo->nWidth, 
                            CV_8UC1, 
                            pBuf);
    ChannelInfo* ch = channelMap[nPort];
    imgQueue.add(std::make_pair(ch -> channel, yv12));
}

void CALLBACK realDataCallBack(LONG lRealHandle, 
                                    DWORD dwDataType, 
                                    BYTE *pBuffer,
                                    DWORD dwBufSize, 
                                    void* dwUser) {
    DWORD dRet;
    LONG nPort;
	ChannelInfo* ch = (ChannelInfo *)dwUser;
    switch (dwDataType) {
    case NET_DVR_SYSHEAD:

	if (!PlayM4_GetPort(&nPort)) {
	    break;
	} else {
        ch -> port = nPort;
        channelMap[nPort] = ch;
	}
	if (dwBufSize > 0) {
	    if (!PlayM4_OpenStream(nPort, pBuffer, dwBufSize, 1024 * 1024)) {
		    dRet = PlayM4_GetLastError(nPort);
            printf("channel %d PlayM4_OpenStrea error: %d\n", 
                    ch -> channel, 
                    dRet);
		    break;
	    }
	    if (!PlayM4_SetDecCallBack(nPort, decodeFunc)) {
		    dRet = PlayM4_GetLastError(nPort);
            printf("channel %d PlayM4_SetDecCallBack error: %d\n", 
                    ch -> channel, 
                    dRet);
		    break;
	    }

	    if (!PlayM4_Play(nPort, 0)) {
		    dRet = PlayM4_GetLastError(nPort);
            printf("channel %d PlayM4_Play error: %d\n", 
                    ch -> channel, 
                    dRet);
		    break;
	    }
	}
	break;

    case NET_DVR_STREAMDATA:
	if (dwBufSize > 0 && ch -> port != -1) {
	    BOOL inData = PlayM4_InputData(ch -> port, pBuffer, dwBufSize);
	    while (!inData) {
		    inData = PlayM4_InputData(nPort, pBuffer, dwBufSize);
	    }
	}
	break;
    }
}

void releaseHandlers(std::map<LONG, ChannelInfo*>& channels) {
    for (auto kv: channels) {
        LONG handler = kv.second -> handler;
        if (handler != -1) {
            LONG result = NET_DVR_StopRealPlay(handler);
            if (result == FALSE) {
                DWORD errorCode = NET_DVR_GetLastError();
                printf("channel %d NET_DVR_StopRealPlay error: %d\n", kv.second -> channel, errorCode);
            }
        }
        delete kv.second;
    }
}

void HaiKangApplication::cameraShot(std::vector<int>& ids, std::string& path) {
    if (ids.empty()) {
        for(unsigned int i = firstChannel; i <= lastChannel; i++) {
            int index = i - firstChannel;
            if (ipParam.struIPChanInfo[index].byEnable){
                ids.push_back(i);
            } else {
                std::cout << i <<  " not enable" << std::endl;
            }
        }
    }
    for (int i: ids) {
        NET_DVR_CLIENTINFO ClientInfo = {0};
        #if (defined(_WIN32) || defined(_WIN_WCE)) || defined(__APPLE__)
        ClientInfo.hPlayWnd     = NULL;
        #elif defined(__linux__)
        ClientInfo.hPlayWnd     = 0;
        #endif

        ClientInfo.lChannel     = i;  //channel NO.
        ClientInfo.lLinkMode    = 0;
        ClientInfo.sMultiCastIP = NULL;

        ChannelInfo* info = new ChannelInfo();
        info -> channel = i;

        LONG handler = NET_DVR_RealPlay_V30(lUserID, &ClientInfo, realDataCallBack, info, 0);
        info -> handler = handler;
        if (handler < 0) {
            DWORD errorCode = NET_DVR_GetLastError();
            printf("channel %d NET_DVR_RealPlay_V30 error: %d\n", i, errorCode);
        } else {
            printf("channel %d NET_DVR_RealPlay_V30 OK\n", i);
        }
    }
    while (results.size() < ids.size()) {
        while (imgQueue.size() != 0) {
            auto img = imgQueue.remove();
            if (results.count(img.first) == 0) {
                results[img.first] = img.second;
                std::string full = path + "/" + std::to_string(img.first) + ".bmp";
                cv::Mat bgr;
                cv::cvtColor(img.second, bgr, CV_YUV2BGR_YV12);
                cv::imwrite(full, bgr);
            }
        }
    }
    
    releaseHandlers(channelMap);
}

bool HaiKangApplication::login() {
    hasInited = NET_DVR_Init();

    NET_DVR_SetLogToFile(3, (char *)logDir.c_str());
        
    NET_DVR_DEVICEINFO_V30 struDeviceInfo = {0};

	lUserID = NET_DVR_Login_V30(const_cast<char *>(host.c_str()), 
                                port, 
                                const_cast<char *>(username.c_str()), 
                                const_cast<char *>(password.c_str()), 
                                &struDeviceInfo);

    if (lUserID < 0) {
        DWORD errorCode = NET_DVR_GetLastError();
        printf("NET_DVR_Login_V30 error: %d\n", errorCode);
        return HPR_ERROR;
    }
    printf("byStartDChan: %d\n", struDeviceInfo.byStartDChan);
    printf("byIPChanNum: %d\n", struDeviceInfo.byIPChanNum);
    printf("byHighDChanNum: %d\n", struDeviceInfo.byHighDChanNum);

    firstChannel = struDeviceInfo.byStartDChan;
    lastChannel = struDeviceInfo.byStartDChan + (struDeviceInfo.byIPChanNum + struDeviceInfo.byHighDChanNum * 256) - 1;
    printf("first channel: %d\n", firstChannel);
    printf("last channel: %d\n", lastChannel);
    NET_DVR_IPPARACFG param;
    if(getIPConInfo(&param, lUserID) ==  HPR_ERROR) {
        return false;
    }
    ipParam = param;
    av_register_all();
    return true;
}

bool HaiKangApplication::init(libconfig::Config& config) {

    if (config.lookupValue(DVR_HOST, host)
        && config.lookupValue(DVR_PORT, port)
        && config.lookupValue(DVR_USERNAME, username)
        && config.lookupValue(DVR_PASSWORD, password)) {
            logInfo("dvr info load OK");
        } else {
            logError("dvr info not complete");
            return false;
        }

    if (!Application::init(config)) {
        return false;
    }
    return true;
}

Channel* HaiKangApplication::createChannel(unsigned int chId, Application* app) {
    return new HaiKangChannel(chId, static_cast<HaiKangApplication*>(app));
}

bool HaiKangApplication::logout() {
    if (lUserID != -1) {
      NET_DVR_Logout_V30(lUserID);
    }
    if (hasInited) {
      NET_DVR_Cleanup();
    }
    return true;
}

HaiKangApplication::~HaiKangApplication() {}