#ifndef _REGION_H_
#define _REGION_H_

#include <vector>

#include <opencv/cv.h>
#include <opencv2/imgproc/imgproc.hpp>

class Region {
public:
    Region(std::string& name, std::vector<cv::Point>& points);
    bool contains(cv::Point2f p);
    ~Region();
    std::string name;
    std::vector<cv::Point> points;
};

#endif