#include <iostream>
#include <vector>
#include <set>
#include <map>

#include <opencv/cv.h>
#include <opencv2/imgproc/imgproc.hpp>

#include <libavutil/pixfmt.h>

#include "HCNetSDK.h"
#include "PlayM4.h"
#include "public.h"

#include "HaiKangChannel.h"
#include "utils.h"


HaiKangChannel::HaiKangChannel(LONG ch, HaiKangApplication* happ): Channel(ch), app(happ) {}

std::map<LONG, HaiKangChannel*> port2channel;

cv::Mat& HaiKangChannel::convert(cv::Mat& yv12, cv::Mat& bgr) {
    if (config -> onGPU) {
        cv::cvtColor(yv12, bgr, CV_YUV2BGR_NV12);
    } else {
        cv::cvtColor(yv12, bgr, CV_YUV2BGR_YV12);
    }
    return bgr;
}

int read_data(void *opaque, uint8_t *buf, int buf_size) {
    HaiKangChannel * ch = (HaiKangChannel *)opaque;
    auto buffer = ch -> bufQueue.remove();
    memcpy(buf, buffer.first, buffer.second);
    delete[] buffer.first;
    return buffer.second;
}

void CALLBACK decodeFunMain(LONG nPort, 
                            char * pBuf, 
                            int nSize, 
                            FRAME_INFO * pFrameInfo, 
                            void* nReserved1, 
                            int nReserved2) {
    cv::Mat yv12 = cv::Mat(pFrameInfo->nHeight * 3 / 2 , 
                            pFrameInfo->nWidth, 
                            CV_8UC1, 
                            pBuf);
    HaiKangChannel* ch = port2channel[nPort];
    if (pFrameInfo -> dwFrameNum % ch -> config -> mod == 0) {
        time_t temp_time = time(0);
        ch -> q.add(std::make_pair(yv12, temp_time));
        ch -> cur_time = temp_time;
    }
    if (ch -> config -> videoOrig) {
        ch -> origQueue.add(yv12);
    }
}

unsigned int bufSize = 1024 * 1024;

void CALLBACK realDataCallBack_V30(LONG lRealHandle, 
                                    DWORD dwDataType, 
                                    BYTE *pBuffer,
                                    DWORD dwBufSize, 
                                    void* dwUser) {
    DWORD dRet;
    LONG nPort;
	HaiKangChannel* ch = (HaiKangChannel *)dwUser;
    switch (dwDataType) {
    case NET_DVR_SYSHEAD:

	if (!PlayM4_GetPort(&nPort)) {
	    break;
	} else {
        ch -> port = nPort;
        port2channel[nPort] = ch;
	}
	if (dwBufSize > 0) {
	    if (!PlayM4_OpenStream(nPort, pBuffer, dwBufSize, bufSize)) {
		    dRet = PlayM4_GetLastError(nPort);
            printf("channel %d PlayM4_OpenStrea error: %d\n", 
                    ch -> channel, 
                    dRet);
		    break;
	    }
	    if (!PlayM4_SetDecCallBack(nPort, decodeFunMain)) {
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
        if (ch -> config -> onGPU) {
            unsigned char* tmp = new unsigned char[dwBufSize];
            memcpy(tmp, pBuffer, dwBufSize);
            ch -> bufQueue.add(std::make_pair(tmp, dwBufSize));
        } else {
	        BOOL inData = PlayM4_InputData(ch -> port, pBuffer, dwBufSize);

	        while (!inData) {
		        inData = PlayM4_InputData(nPort, pBuffer, dwBufSize);
	        }
        }
	}
	break;
    }
}

bool HaiKangChannel::runFFmpeg() {
    buf = new unsigned char[bufSize];
    pb = avio_alloc_context(buf, bufSize, 0, this, read_data, NULL, NULL);

    if (!pb) {
        fprintf(stderr, "avio_alloc_context failed!\n");
        return false;
    }

    if (av_probe_input_buffer(pb, &piFmt, "", NULL, 0, 0) < 0) {
        fprintf(stderr, "probe failed!\n");
        return false;
    } else {
        fprintf(stdout, "probe success!\n");
        fprintf(stdout, "format: %s[%s]\n", piFmt -> name, piFmt -> long_name);
    }

    pFmt = avformat_alloc_context();
    pFmt -> pb = pb;

    if (avformat_open_input(&pFmt, "", piFmt, NULL) < 0) {
        fprintf(stderr, "avformat open failed.\n");
        return false;
    } else {
        fprintf(stdout, "open stream success!\n");
    }
    
    AVDictionary* pOptions = NULL;
    if (avformat_find_stream_info(pFmt, &pOptions) < 0) {
        fprintf(stderr, "could not fine stream.\n");
        return false;
    }

    av_dump_format(pFmt, 0, "", 0);

    int videoIndex = -1;  
    for (unsigned int i = 0; i < pFmt->nb_streams; i++) {
        if(pFmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {  
            videoIndex = i;  
            break;  
        }
    }

    if (videoIndex == -1) {  
        printf("Didn't find a video stream.\n");  
        return false;  
    }

    pCodecCtx = avcodec_alloc_context3(NULL);
    if (pCodecCtx == NULL) {  
        printf("Could not allocate AVCodecContext\n");  
        return false;  
    }
    avcodec_parameters_to_context(pCodecCtx, pFmt->streams[videoIndex]->codecpar); 
    AVCodec *pCodec = avcodec_find_decoder_by_name("h264_cuvid");
    if (pCodec == NULL) {  
        printf("Codec not found.\n");  
        return false;  
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {  
        printf("Could not open codec.\n");  
        return false;  
    }

    int got_picture;
    AVFrame *pframe = av_frame_alloc();
    AVPacket pkt;
    av_init_packet(&pkt);
    unsigned long long index = 0;
    int video_decode_size = avpicture_get_size(AV_PIX_FMT_NV12, pCodecCtx->width, pCodecCtx->height);
    uint8_t * video_decode_buf = new uint8_t[video_decode_size];
    while (running) {
        if (av_read_frame(pFmt, &pkt) >= 0) {
            if (pkt.stream_index == videoIndex) {
                avcodec_decode_video2(pCodecCtx, pframe, &got_picture, &pkt);
                if (got_picture) {
                    int i,j;
                    for (i = 0; i < pCodecCtx->height; i++) {
                        memcpy(video_decode_buf + pCodecCtx->width * i,
                                pframe->data[0] + pframe->linesize[0] * i,
                                pCodecCtx->width);
                    }

                    for (j = 0; j < pCodecCtx->height / 2; j++) {
                        memcpy(video_decode_buf + pCodecCtx->width * i + pCodecCtx->width * j,
                                pframe->data[1] + pframe->linesize[1] * j,
                                pCodecCtx->width);
                    }
                    cv::Mat yv12 = cv::Mat(pframe->height * 3 / 2 , 
                                            pframe->width, 
                                            CV_8UC1, 
                                            video_decode_buf);
                    if (index % config -> mod == 0) {
                        time_t temp_time = time(0);
                        q.add(std::make_pair(yv12, temp_time));
                        cur_time = temp_time;

                    }
                    if (config -> videoOrig) {
                        origQueue.add(yv12);
                    }
                    index++;
                    
                }
            }
        } else {
            break;
        }
    }
    delete [] video_decode_buf;
    av_frame_free(&pframe);
    av_packet_unref(&pkt);
    return true;
}

bool HaiKangChannel::stop() {
    Channel::stop();
    if (handler != -1) {
        LONG result = NET_DVR_StopRealPlay(handler);
        if (result == FALSE) {
            DWORD errorCode = NET_DVR_GetLastError();
            printf("channel %d NET_DVR_StopRealPlay error: %d\n", channel, errorCode);
            return false;
        }
    }

    return true;
}

bool HaiKangChannel::start() {
    if (NET_DVR_SetReconnect(2000, TRUE) == FALSE) {
        DWORD errorCode = NET_DVR_GetLastError();
        printf("channel %d NET_DVR_SetReconnect error: %d\n", channel, errorCode);
        return false;
    }
    NET_DVR_CLIENTINFO ClientInfo = {0};
#if (defined(_WIN32) || defined(_WIN_WCE)) || defined(__APPLE__)
    ClientInfo.hPlayWnd     = NULL;
#elif defined(__linux__)
   ClientInfo.hPlayWnd     = 0;
#endif

    ClientInfo.lChannel     = channel;  //channel NO.
    ClientInfo.lLinkMode    = 0;
    ClientInfo.sMultiCastIP = NULL;

    handler = NET_DVR_RealPlay_V30(app -> lUserID, &ClientInfo, realDataCallBack_V30, this, 0);
    if (handler < 0) {
        DWORD errorCode = NET_DVR_GetLastError();
        printf("channel %d NET_DVR_RealPlay_V30 error: %d\n", channel, errorCode);
        return false;
    }
    Channel::start();
    if (config -> onGPU) {
        ffmpegDecodeThread = std::thread(&HaiKangChannel::runFFmpeg, this);
    }
    return true;
}

HaiKangChannel::~HaiKangChannel() {
    if(buf) {
        delete[] buf;
    }
    if(pb) {
        av_free(pb);
    }
    if(piFmt) {
	av_free(piFmt);
    }
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFmt);  
}
