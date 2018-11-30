#ifndef _DaHuaChannel_H_
#define _DaHuaChannel_H_
#include <map>
#include <set>
#include <queue>
#include <thread>
#include <atomic>

#include "CVModel.h"
#include "Channel.h"
#include "DaHuaApplication.h"

class DaHuaChannel: public Channel {

public:
    DaHuaChannel(LONG ch, DaHuaApplication*);
    bool start() override;
    bool stop() override;
    ~DaHuaChannel() override;

    DaHuaApplication* app;
    LONG handler = 0;
    LONG port = 0;
private:
    cv::Mat& convert(cv::Mat&, cv::Mat&) override;
};
#endif
