#ifndef _CHANNEL_H_
#define _CHANNEL_H_
#include <map>
#include <queue>
#include <thread>
#include <atomic>
#include <opencv2/freetype.hpp>

#include "public.h"
#include "CVModel.h"
#include "Logging.h"
#include "Region.h"
#include "Target.h"
#include "DB.h"
#include "FrameInfo.h"
#include "Config.h"
#include "VideoWriter.h"
#include "wqueue.h"
#include "time.h"
typedef std::map<std::string, std::vector<cv::Rect>> RegionMap;

class Channel: public Logging {



public:
    Channel(LONG ch);
    virtual bool start();
    virtual bool stop();
    virtual void join();
    virtual ~Channel();
    virtual bool init(libconfig::Setting& config);
    virtual void post(cv::Mat& src, time_t&);

    wqueue<cv::Mat> origQueue;
    wqueue<std::pair<cv::Mat, time_t>> q;
    std::atomic<bool> running;
    std::atomic<time_t> cur_time = {(time_t) 0};
    LONG channel;
    std::string name;
    std::string configName;
    std::vector<Region> regions;
    std::string stationtype;
    int in_orientation = 1;
    bool readHaikang = false;
    FrameInfo lastFrame;

    Configuration* config = nullptr;

    long long avgtime = 0;
    long long count = 0;

    DB db;

    VideoWriter* writer = nullptr;

private:

    FrameInfo nextFrame (RegionMap& regionMap, FrameInfo& lastFrame, cv::Mat& src);
    void render(cv::Mat& src, time_t& time);
    virtual cv::Mat& convert(cv::Mat&, cv::Mat&) = 0;


    std::thread t;
    std::thread origThread;
    void doRun(cv::Mat&);
    void run();
    void origRun();

    cv::Ptr<cv::freetype::FreeType2> ft2 = cv::freetype::createFreeType2();
};
#endif
