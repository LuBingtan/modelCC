#ifndef _HaiKangChannel_H_
#define _HaiKangChannel_H_
#include <map>
#include <set>
#include <queue>
#include <thread>
#include <atomic>


#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h> 
#include <libavformat/avformat.h>
#ifdef __cplusplus 
}
#endif

#include "public.h"
#include "CVModel.h"
#include "Channel.h"
#include "HaiKangApplication.h"

class HaiKangChannel: public Channel {

public:
    HaiKangChannel(LONG ch, HaiKangApplication*);
    bool start() override;
    bool stop() override;
    ~HaiKangChannel() override;
    bool runFFmpeg();

    HaiKangApplication* app;
    LONG handler;
    LONG port;

    AVIOContext* pb = nullptr;
    AVInputFormat *piFmt = nullptr;
    AVFormatContext *pFmt = nullptr;
    AVCodecContext *pCodecCtx = nullptr;

    unsigned char* buf = nullptr;

    wqueue<std::pair<unsigned char*, unsigned int>> bufQueue;

private:
    cv::Mat& convert(cv::Mat&, cv::Mat&) override;
    std::thread ffmpegDecodeThread;
};
#endif
