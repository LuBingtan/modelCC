#ifndef _TARGET_H_
#define _TARGET_H_

#include <vector>

#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "Region.h"
#include "TargetInfo.h"
#include "Cmt.h"
#include "Logging.h"

const int IN = 0; // last frame is OUT_OF_ANY_REGION, this frame is in a region, we name it IN;
const int STAY = 1; // last frame is in a region, this frame is in the same region, we name it STAY;
const int OUT = 2; // last frame is in a region, this frame OUT_OF_ANY_REGION, we name it OUT;
const int ENTER = 3;
const int EXIT = 4;

class Target {
public:
    Target();
    cv::Point center() const;
    cv::Point pleft();
    ~Target();
    std::string id;
    int state;
    std::string channel;
    std::string region;
    std::vector<std::string> license;
    std::vector<std::string> type;
    std::vector<std::string> color;
    std::vector<std::string> make;
    std::vector<cv::Point> car_coordinate;
    int stayCount;
    int outCount;
    cv::Rect rect;
    cv::Rect prect;
    bool operator <(const Target& t) const;
    std::string toString() const;
    std::string maxOccurrences(std::vector<std::string>& vs) const;
    TargetInfo toTargetInfo();
    bool isOut();
    void addCmt(CMT& c);
    void addCoordinate(cv::Rect& r);
    void stayOnce(cv::Rect& r, int stayCountInterval, cv::Mat& src);
    void outOnce();
    void markOut();
    void markIn();
    void markStatus(int in_orientation);
    void updateOcr(std::pair<std::string, cv::Rect> ocr);
    void setup(std::string channel, std::string region, cv::Rect& rect);
};

#endif