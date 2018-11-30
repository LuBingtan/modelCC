#ifndef _MOCKCHANNEL_H_
#define _MOCKCHANNEL_H_

#include <map>
#include <set>
#include <queue>
#include <thread>
#include <atomic>
#include <fstream>

#include <opencv/cv.h>
#include <opencv2/highgui/highgui.hpp> 

#include "public.h"
#include "CVModel.h"
#include "Channel.h"
#include "MockApplication.h"
#include "HttpGet.h"

class LabeledData {
public:
    LabeledData(int, int, std::chrono::seconds, std::string);
    int pumpId;
    int pumpStatus;
    std::chrono::seconds time;
    std::string license;
    std::string key();
};

class MockChannel: public Channel {

public:
    MockChannel(LONG ch, MockApplication*);
    bool start() override;
    ~MockChannel() override;
    void join() override;
    bool init(libconfig::Setting& config) override;
    void post(cv::Mat& src, time_t& time) override;

private:
    cv::Mat& convert(cv::Mat&, cv::Mat&) override;
    time_t base;
    int mockFps = -1;
    int pass = 0;
    int fail = 0;
    std::thread readThread;
    std::thread curlThread;
    wqueue<std::string> curlQueue;
    std::map<std::string, std::vector<LabeledData>> mockData;
    std::queue<std::pair<std::string, std::string>> mockVideos;
    void loadLabeledData(std::string& filename);
    void run();
    void read();
    void curlRun();
    HttpGet get;
    void cleanup();
    MockApplication* app = nullptr;
    bool enable = false;
    //opencv decoding
    bool read_opencv(std::string name);
    //haikang sdk decoding
    bool read_haikang(std::string name);
};
#endif
