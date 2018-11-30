#ifndef _VIDEO_WRITER_H_
#define _VIDEO_WRITER_H_

#include <opencv/cv.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>

#include "Config.h"

class VideoWriter {
public:
    VideoWriter(bool orig, bool debug, std::string& path, std::string& channel);
    void writeOrigVideo(cv::Mat& src);
    void writeDebugVideo(cv::Mat& src);
    int close();
    ~VideoWriter();
    int keepDays = 3;

private:
    void writeVideo(cv::Mat& src, 
                    cv::VideoWriter** writerp, 
                    const std::string surfix, 
                    std::string* currentHour, 
                    bool enable, int fps);
    cv::VideoWriter* origWriter = nullptr;
    cv::VideoWriter* debugWriter = nullptr;
    bool writeOrig = false;
    bool writeDebug = false;
    bool inited = false;
    std::string baseDir;
    std::string ch;
    std::string currentHourDebug;
    std::string currentHourOrig;
};

#endif
