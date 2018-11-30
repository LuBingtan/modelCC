#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <functional>
#include <ctime> 
#include <opencv/cv.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <random>


#include "utils.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"

std::vector<std::string> split(const std::string &s, const std::string &seperator) {
  std::vector<std::string> result;
  typedef std::string::size_type string_size;
  string_size i = 0;
  
  while (i != s.size()) {
    int flag = 0;
    while (i != s.size() && flag == 0) {
      flag = 1;
      for (string_size x = 0; x < seperator.size(); ++x) {
      if (s[i] == seperator[x]) {
        ++i;
        flag = 0;
          break;
        }
      }
    }
    
    flag = 0;
    string_size j = i;
    while (j != s.size() && flag == 0) {
      for (string_size x = 0; x < seperator.size(); ++x) {
        if (s[j] == seperator[x]) {
            flag = 1;
            break;
        }
      }

      if(flag == 0) {
        ++j;
      }
    }
    if (i != j) {
      result.push_back(s.substr(i, j-i));
      i = j;
    }
  }
  return result;
}

inline std::string& lTrim(std::string &ss) {
    std::string::iterator p = find_if(ss.begin(),ss.end(),std::not1(std::ptr_fun(isspace)));   
    ss.erase(ss.begin(),p);   
    return ss;   
}

inline std::string& rTrim(std::string &ss) {
  std::string::reverse_iterator p = find_if(ss.rbegin(),ss.rend(),std::not1(std::ptr_fun(isspace)));   
  ss.erase(p.base(),ss.end());   
  return ss;   
}

std::string& trim(std::string &st) {
  lTrim(rTrim(st));   
  return st;   
}

tensorflow::Tensor convert_uint8(cv::Mat& image) {
    int height = image.rows;
    int width = image.cols;
    int depth = image.channels();
    tensorflow::Tensor input_tensor(tensorflow::DT_UINT8, 
        tensorflow::TensorShape({1, height, width, depth}));
    auto p = input_tensor.flat<unsigned char>().data();

  cv::Mat converted(height, width, CV_8UC3, p);
  image.convertTo(converted, CV_8UC3);

  return input_tensor;
}

tensorflow::Tensor convert_float(cv::Mat& image, int type) {
    int height = image.rows;
    int width = image.cols;
    int depth = image.channels();
    tensorflow::Tensor input_tensor(tensorflow::DT_FLOAT, 
        tensorflow::TensorShape({1, height, width, depth}));
    auto p = input_tensor.flat<float>().data();

  cv::Mat converted(height, width, type, p);
  image.convertTo(converted, type);

  return input_tensor;
}

tensorflow::Tensor convert2(cv::Mat& image) {
    int height = image.rows;
    int width = image.cols;
    int depth = image.channels();
    tensorflow::Tensor input_tensor(tensorflow::DT_FLOAT, 
        tensorflow::TensorShape({1, height, width, depth}));
    auto input_tensor_mapped = input_tensor.tensor<float, 4>();

  cv::Mat converted;

  const unsigned char* source_data = (unsigned char*) image.data;

  for (int y = 0; y < height; ++y) {
    const unsigned char* source_row = source_data + (y * width * depth);
    for (int x = 0; x < width; ++x) {
      const unsigned char* source_pixel = source_row + (x * depth);
      for (int c = 0; c < depth; ++c) {
        const unsigned char* source_value = source_pixel + c;
        input_tensor_mapped(0, y, x, c) = (float) (*source_value);
      }
    }
  }
  return input_tensor;
}

std::string random_string(size_t length) {
    static auto& chrs = "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    thread_local static std::mt19937 rg{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    std::string s;

    s.reserve(length);

    while(length--)
        s += chrs[pick(rg)];

    return s;
}
std::string ttos(time_t t, std::string timeFormat) {
        const size_t bufferSize = 20;
        char buff[bufferSize];
        struct tm timeMake = {0};
        localtime_r(&(t), &timeMake);
        strftime(buff, bufferSize, timeFormat.c_str(), &timeMake);
        std::string timeString = buff;
        return timeString;
}

std::string time_t_to_string_hour(time_t t) {
    return ttos(t, "%Y-%m-%d-%H");
}

std::string time_t_to_string(time_t t) {
    return ttos(t, "%Y-%m-%d %H:%M:%S");
}


time_t string_to_time_t(std::string time_details) {
  struct tm tm;
  strptime(time_details.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
  time_t t = mktime(&tm);
  return t;
}
