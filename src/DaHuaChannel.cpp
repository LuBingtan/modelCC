#include <iostream>
#include <vector>
#include <set>
#include <map>

#include <opencv/cv.h>
#include <opencv2/imgproc/imgproc.hpp>

#include "DaHuaChannel.h"

#include "DHinclude/dhnetsdk.h"
#include "DHinclude/dhplay.h"

DaHuaChannel::DaHuaChannel(LONG ch, DaHuaApplication* happ): Channel(ch), app(happ) {}

std::map<LONG, DaHuaChannel*> port2channel_DH;

cv::Mat& DaHuaChannel::convert(cv::Mat& iyuv, cv::Mat& bgr) {
    cv::cvtColor(iyuv, bgr, CV_YUV2BGR_IYUV);
    return bgr;
}

void CALLBACK decodeFunMain_DH(LONG nPort, 
                            char * pBuf, 
                            int nSize, 
                            FRAME_INFO * pFrameInfo, 
                            void* nReserved1, 
                            LONG nReserved2) {
	if(pFrameInfo->nType != T_IYUV)
		return;
    cv::Mat iyuv = cv::Mat(pFrameInfo->nHeight *3 / 2, 
                            pFrameInfo->nWidth, 
                            CV_8UC1, 
                            pBuf);
	
    DaHuaChannel* ch = port2channel_DH[nPort];
	long frame_num = pFrameInfo->nFrameRate * pFrameInfo->nStamp / 1000;

    if (frame_num % ch -> config -> mod == 0) {
		time_t temp_time = time(0);
    	ch -> q.add(std::make_pair(iyuv, temp_time));
		ch -> cur_time = temp_time;
    }
    if (ch -> config -> videoOrig) {
        ch -> origQueue.add(iyuv);
    }
}

void CALLBACK RealDataCallBackEx(LLONG lRealHandle, DWORD dwDataType, BYTE *pBuffer,DWORD dwBufSize, LONG lParam, LDWORD dwUser)
{
	DaHuaChannel* ch = (DaHuaChannel *)dwUser;
	port2channel_DH[ch->port] = ch;
	BOOL bInput=FALSE;
	if(0 != lRealHandle)
	{
		switch(dwDataType) {
		case 0:
			//原始音视频混合数据
			bInput = PLAY_InputData(ch->port,pBuffer,dwBufSize);
			if (!bInput)
			{
				printf("input data error: %d\n", PLAY_GetLastError(ch->port));
			}
			break;
		case 1:
			//标准视频数据
			break;
		case 2:
			//yuv 数据
			break;
		case 3:
			//pcm 音频数据
			
			break;
		case 4:
			//原始音频数据
			
			break;
		default:
			break;
		}	
	}
}
bool DaHuaChannel::stop() {
    Channel::stop();
    if (handler != 0) {
		//关闭解码库播放
		PLAY_Stop(port);
		//释放播放库
		PLAY_CloseStream(port);
		//关闭预览
		CLIENT_StopRealPlayEx(app -> lUserID);
    }
    return true;
}
bool DaHuaChannel::start() {
	if(app -> lUserID != 0) {
		if(!PLAY_GetFreePort(&port)) {
			printf("PLAY_GetFreePort failed.\n");
			return false;
		}
		printf("PLAY_GetFreePort: %d\n", port);
		if(!PLAY_SetStreamOpenMode(port, STREAME_REALTIME)) {
			printf("PLAY_SetStreamOpenMode failed.\n");
			return false;
		}
		if(PLAY_OpenStream(port, 0, 0, 1024*500)) {
			printf("PLAY_OpenStream.\n");
			if(!PLAY_SetDecCallBack(port, decodeFunMain_DH)) {
				printf("PLAY_SetDecCallBack failed.\n");
				return false;
			}
			PLAY_SetDecCBStream(port,1);
			if(PLAY_Play(port,NULL)) {
				printf("PLAY_Play.\n");
				handler = CLIENT_RealPlayEx(app -> lUserID, channel, NULL, DH_RType_Realplay);
				if (handler != 0) {
					printf("CLIENT_RealPlayEx.\n");
					if(!CLIENT_CloseSound()) {
						printf("CLIENT_CloseSound failed.\n");
						return false;
					}
					if(!CLIENT_SetRealDataCallBackEx(handler, RealDataCallBackEx, (LDWORD)this, 0x1f)) {
						printf("CLIENT_SetRealDataCallBackEx failed.\n");
						return false;
					}
				} else {
					DWORD errorCode = CLIENT_GetLastError();
					printf("channel %d CLIENT_RealPlayEx error: %d\n", channel, errorCode);
					PLAY_Stop(port);
					PLAY_CloseStream(port);
					return false;
				}
			} else {
				printf("PLAY_Play failed. Error code: %d\n", PLAY_GetLastError(port));
				PLAY_CloseStream(port);
				return false;
			}
		} else {
			printf("PLAY_OpenStream failed. Error code: %d\n", PLAY_GetLastError(port));
			return false;
		}
		Channel::start();
		printf("DaHua Channel start success\n");
		return true;
	} else {
		printf("DaHua Channel start failed: lUserID is 0.\n");
		return false;
	}
}
DaHuaChannel::~DaHuaChannel() {

}
