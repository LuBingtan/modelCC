#ifndef _UTILS_H_
#define _UTILS_H_

#include <opencv/cv.h>

#include "public.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"

std::vector<std::string> split(const std::string &s, const std::string &seperator);
std::string& trim(std::string &st);
tensorflow::Tensor convert_uint8(cv::Mat& bgr);
tensorflow::Tensor convert_float(cv::Mat& bgr, int type);
tensorflow::Tensor convert2(cv::Mat& image);
std::string random_string(size_t length);
std::string time_t_to_string(time_t t);
std::string time_t_to_string_hour(time_t t);
time_t string_to_time_t(std::string t);
#endif
