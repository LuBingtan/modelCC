#include "Region.h"
Region::Region(std::string& name, std::vector<cv::Point>& points): name(name), points(points) {}

bool Region::contains(cv::Point2f p) {
    return pointPolygonTest(points, p, false) > 0;
}

Region::~Region() {}