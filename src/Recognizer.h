#ifndef _RECOGNIZER_H_
#define _RECOGNIZER_H_

#include <map>
#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <libconfig.h++>

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"
#include "Model.h"
#include "Logging.h"
#include "utils.h"

class Recognizer: public Logging {
public:
    Recognizer(std::string name);

    void load(libconfig::Config& config, const std::string& key);

    template <typename T> std::vector<T> predict(cv::Mat& bgr, cv::Rect& rect, bool cvtGray) {
        cv::Mat mat(bgr, rect);
        cv::Mat resized;
        tensorflow::Tensor t;

        if (cvtGray) {
            cv::Mat gray;
            cv::cvtColor(mat, gray, CV_BGR2GRAY);
            cv::resize(gray, resized, cv::Size(width, height));
            t = convert_float(resized, CV_32FC1);
        } else {
            cv::resize(mat, resized, cv::Size(width, height));
            t = convert_float(resized, CV_32FC3);
        }

        std::vector<tensorflow::Tensor> result = model.predict(t);

        std::vector<T> info;

        for (unsigned int i = 0 ; i < result.size(); i++) {
            auto prediction = result[i].flat_inner_dims<T>();
            for (unsigned int i = 0 ; i < prediction.size(); i++) {
                info.push_back(prediction(i));
            }
        }
        return info;
    };

    ~Recognizer();

    Model model;
    
    float threshold;

    int height;

    int width;
};
#endif
