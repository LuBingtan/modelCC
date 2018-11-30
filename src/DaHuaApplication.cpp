#include <map>

#include "DaHuaApplication.h"
#include "DaHuaChannel.h"
#include "Config.h"
#include "wqueue.h"
#include "DHinclude/dhnetsdk.h"
#include "DHinclude/dhplay.h"
const std::string DVR_HOST = "dvr.host";
const std::string DVR_PORT = "dvr.port";
const std::string DVR_USERNAME = "dvr.username";
const std::string DVR_PASSWORD = "dvr.password";

const std::string logDir = "log";

DaHuaApplication::DaHuaApplication(): Application("DaHuaApplication") {}




//camerashot
int chNum_camera = 0;
wqueue<std::pair<int, cv::Mat>> imgQueue_DH;
std::map<int,int> port_channel;
std::map<int,bool> channel_rst;
void CALLBACK decodeFunMain_DH_camera(LONG nPort, char * pBuf, int nSize, FRAME_INFO * pFrameInfo, void* nReserved1, LONG nReserved2) {
	if(pFrameInfo->nType !=3 ) {
		printf("Decode method error: data type is not T_IYUV---%d.---\n",pFrameInfo->nType);
		return;
	}
	if(channel_rst.count(port_channel[nPort]))
		return;
	channel_rst.insert(std::make_pair(nPort, port_channel[nPort]));
	cv::Mat iyuv = cv::Mat(pFrameInfo->nHeight *3 / 2, pFrameInfo->nWidth, CV_8UC1, pBuf);
	imgQueue_DH.add(std::make_pair(port_channel[nPort],iyuv));
	return;
}
void CALLBACK RealDataCallBackEx_DH_camera(LLONG lRealHandle, DWORD dwDataType, BYTE *pBuffer,DWORD dwBufSize, LONG lParam, LDWORD dwUser) {
	if(dwDataType == 0) {
		PLAY_InputData((int)dwUser,pBuffer,dwBufSize);
	}
	return;
}
void DaHuaApplication::cameraShot(std::vector<int>& ids, std::string& path) {
	if(ids.empty()) {
		for(int i=0; i<chNum_camera; i++) {
			ids.push_back(i);
		}
	}
	LONG handler = 0;
	for(auto i: ids){
		int port = 0;
		PLAY_GetFreePort(&port);
		port_channel.insert(std::make_pair(port,i));
		if(!PLAY_SetStreamOpenMode(port, STREAME_REALTIME)) {
			printf("PLAY_SetStreamOpenMode failed.\n");
			return;
		}
		if(!PLAY_OpenStream(port, 0, 0, 1024*500)) {
			printf("PLAY_OpenStream failed\n");
			return;
		}
		PLAY_SetDecCallBack(port, decodeFunMain_DH_camera);
		PLAY_SetDecCBStream(port,1);
		if(!PLAY_Play(port,NULL)) {
			printf("PLAY_Play failed.\n");
			PLAY_CloseStream(port);
			return;
		}
		handler = CLIENT_RealPlayEx(lUserID, i, NULL, DH_RType_Realplay);
		if(handler == 0) {
			printf("CLIENT_RealPlayEx failed. Error code: %d.\n",CLIENT_GetLastError());
			PLAY_Stop(port);
			PLAY_CloseStream(port);
			return;
		}
		CLIENT_CloseSound();
		CLIENT_SetRealDataCallBackEx(handler, RealDataCallBackEx_DH_camera, (LDWORD)port, 0x1f);
	}

	while (channel_rst.size() < port_channel.size()) {
		while(imgQueue_DH.size()>0) {
			auto img = imgQueue_DH.remove();
			std::string full = path + "/" + std::to_string(img.first) + ".bmp";
			cv::Mat bgr;
            cv::cvtColor(img.second, bgr, CV_YUV2BGR_IYUV);
            cv::imwrite(full, bgr);
		}
	}
	PLAY_Stop(port);
	PLAY_CloseStream(port);
	CLIENT_StopRealPlay(handler);
}

//断线回调函数
void CALLBACK DisConnectFunc(LLONG lLoginID, char *pchDVRIP, LONG nDVRPort, LDWORD dwUser) {
	printf("DisConnect.\n");
}

//自动重连回调函数
void CALLBACK HaveReConnectFunc(LLONG lLoginID, char *pchDVRIP, LONG nDVRPort, LDWORD dwUser) {
	printf("Device reconnect, IP=%s, Port=%d\n", pchDVRIP, nDVRPort);
}

//子连接自动重连回调函数
void CALLBACK SubHaveReConnectFunc(EM_INTERFACE_TYPE emInterfaceType, BOOL bOnline, LLONG lOperateHandle, LLONG lLoginID, LDWORD dwUser) {
	printf("Device reconnecte.\n");
	switch(emInterfaceType)
	{
	case DH_INTERFACE_REALPLAY:
		printf("实时监视接口: Short connect is %d\n", bOnline);
		break;
	case DH_INTERFACE_PREVIEW:
		printf("多画面预览接口: Short connect is %d\n", bOnline);
		break;
	case DH_INTERFACE_PLAYBACK:
		printf("回放接口: Short connect is %d\n", bOnline);
	     break;
	case DH_INTERFACE_DOWNLOAD:
		printf("下载接口: Short connect is %d\n", bOnline);
	     break;
	default:
	     break;
	}
}

bool DaHuaApplication::login() {
	CLIENT_LogClose();
	printf("Login Begin...\n");
	//初始化SDK
	CLIENT_Init(DisConnectFunc, 0);
	printf("CLIENT_Init\n");
	hasInited=true;
	//设置连接参数
	CLIENT_SetConnectTime(3000, 3);
	printf("CLIENT_SetConnectTime.\n");
	//设置自动重连回调函数
	CLIENT_SetAutoReconnect(HaveReConnectFunc, 0);
	printf("CLIENT_SetAutoReconnect\n");
	//设置子连接自动重连回调函数
	CLIENT_SetSubconnCallBack(SubHaveReConnectFunc, 0);
	printf("CLIENT_SetSubconnCallBack\n");
	NET_DEVICEINFO stDevInfo = {0};
	
	int nError = 0;
	//注册用户到设备
	lUserID = CLIENT_Login(const_cast<char *>(host.c_str()),
								port,
								const_cast<char *>(username.c_str()),
								const_cast<char *>(password.c_str()),
								&stDevInfo, &nError);
	if(lUserID == 0) {
		printf("NET_DEVICEINFO error: %d\n", nError);
		return false;
	}
	printf("CLIENT_Login: Done.\n");
	printf("Device Serial Number: %d\n", stDevInfo.sSerialNumber);
	printf("Device Channel Number: %d\n", stDevInfo.byChanNum);
	chNum_camera = (int)stDevInfo.byChanNum;
	return true;
}
bool DaHuaApplication::logout() {
	if(lUserID != 0) {
		CLIENT_Logout(lUserID);
		printf("CLIENT_Logout: Done.\n");
	}
	if(hasInited) {
		CLIENT_Cleanup();
		printf("CLIENT_Cleanup: Done.\n");
	}
	
	return true;
}

bool DaHuaApplication::init(libconfig::Config& config) {

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

Channel* DaHuaApplication::createChannel(unsigned int chId, Application* app) {
    return new DaHuaChannel(chId, static_cast<DaHuaApplication*>(app));
}
DaHuaApplication::~DaHuaApplication() {}