#ifndef _DETECTOR_H_
#define _DETECTOR_H_
#include <map>
#include <set>
#include <opencv/cv.h>

#include <libconfig.h++>

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"
#include "Model.h"
#include "Logging.h"

class Detector: public Logging {
public:
    Detector(std::string name);

    void load(libconfig::Config& config, const std::string& key);

    std::vector<cv::Rect> predict(cv::Mat& bgr, cv::Rect& rect);

    ~Detector();

    Model model;
    
    float threshold;
};
#endif
