#include <iostream>
#include <vector>
#include <map>

#include <cstdlib>

#include "CVModel.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include "Model.h"

CVModel::CVModel(): vehicle("vehicle"), 
                    license("license"),
                    ocr("ocr"),
                    colorMakeType("cmt"),
                    digital("digital") {
}

CVModel::~CVModel() {}

const std::string MODEL_VEHICLE = "model.vehicle";
const std::string MODEL_LICENSE = "model.license";
const std::string MODEL_OCR = "model.ocr";
const std::string MODEL_CMT = "model.cmt";
const std::string MODEL_DIGITAL = "model.digital";

const std::string COLOR = "color";
const std::string MAKE = "make";
const std::string TYPE = "type";

const int COLORI = 0;
const int MAKEI = 1;
const int TYPEI = 2;

int CVModel::init(libconfig::Config& config) {
    vehicle.load(config, MODEL_VEHICLE);
    license.load(config, MODEL_LICENSE);
    ocr.load(config, MODEL_OCR);
    colorMakeType.load(config, MODEL_CMT);

    return 0;
}

int CVModel::loadDict(libconfig::Config& config) {
    libconfig::Setting& colorSetting = config.lookup(COLOR);
    libconfig::Setting& makeSetting = config.lookup(MAKE);
    libconfig::Setting& typeSetting = config.lookup(TYPE);
    for (int i = 0; i < colorSetting.getLength(); i++) {
        color.push_back(colorSetting[i]);
    }
    for (int i = 0; i < makeSetting.getLength(); i++) {
        make.push_back(makeSetting[i]);
    }
    for (int i = 0; i < typeSetting.getLength(); i++) {
        type.push_back(typeSetting[i]);
    }
    return 0;
}

cv::Rect extendRect(cv::Rect& rect, cv::Mat& src, int margin) {
    int x = std::max(rect.x - margin, 0);
    int y = std::max(rect.y - margin, 0);
    int w = std::min(rect.width + margin * 2, src.cols - x);
    int h = std::min(rect.height + margin * 2, src.rows - y);

    cv::Rect extend(x, y, w, h);
    return extend;
}

const int margin = 3;

std::pair<std::string, cv::Rect> CVModel::predictLicense(cv::Rect& rect, cv::Mat& src) {
    rect = extendRect(rect, src, margin);
    auto licenseResult = license.predict(src, rect);
    std::string resString;
    cv::Rect extl;
    if (!licenseResult.empty()) {
        extl = extendRect(licenseResult.front(), src, margin);

        auto ocrResult = ocr.predict<std::string>(src, extl, true);
        
        for (auto s: ocrResult) {
            resString.append(s);
        }
    }

    return std::make_pair(resString, extl);
}

CMT CVModel::predictColorMakeType(cv::Rect& r, cv::Mat& src) {

    cv::Mat mat(src, r);
    cv::Mat rgb;

    cv::cvtColor(mat, rgb, CV_BGR2RGB);
    cv::Rect rgbRect(0, 0, rgb.cols - 1, rgb.rows - 1);

    auto colorMakeTypeResult = colorMakeType.predict<long long int>(rgb, rgbRect, false);
    CMT result;
    result.color = color[colorMakeTypeResult[COLORI]];
    result.make = make[colorMakeTypeResult[MAKEI]];
    result.type = type[colorMakeTypeResult[TYPEI]];
    return result;
}

inline void predictOneDigital(cv::Mat& src, cv::Rect rect, std::string& result) {
    auto m1 = CVModel::GetInstance().digital.predict<long long int>(src, rect, true);
    for (auto s: m1) {
        result.append(std::to_string(s));
    }
}

std::string CVModel::getTime(cv::Mat& src) {
  cv::Point start(1086, 6);
  int width = 32;
  int height = 44;
  int interval = 0;

  std::string result;
  // month
  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  interval += width; //jump -
  result.append("-");

  // day
  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  interval += width; //jump -
  result.append("-");

  // year
  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  interval += width * 8; //jump weekday
  result.append(" ");

  // hour
  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  interval += width; //jump :
  result.append(":");

  // min
  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  interval += width; //jump :
  result.append(":");

  // second
  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  cv::Rect(start.x + interval, start.y, width, height);
  predictOneDigital(src, cv::Rect(start.x + interval, start.y, width, height), result);
  interval += width;

  return result;
}
std::string CVModel::predictColor(cv::Rect& rect, cv::Mat& src)
{
    cv::Mat mat(src, rect);
    LicenceColorRecognizer::LicenceColor color = color_recognizer.recognize(mat, false);
    std::string color_str;
    switch(color)
    {
        case LicenceColorRecognizer::BLUE:
            color_str = "blue";
            break;
        case LicenceColorRecognizer::GREEN:
            color_str = "green";
            break;
        case LicenceColorRecognizer::YELLOW:
            color_str = "yellow";
            break;
        case LicenceColorRecognizer::BLACK:
            color_str = "black";
            break;
        case LicenceColorRecognizer::WHITE:
            color_str = "white";
            break;
    }
    return color_str;
}