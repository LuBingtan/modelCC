#include <iostream>
#include <vector>
#include <map>

#include <opencv/cv.h>

#include "Detector.h"
#include "utils.h"

Detector::Detector(std::string name): Logging(name + "Detector"), model(name) {
}


const std::string PATH = "path";
const std::string THRESHOLD = "threshold";
const std::string INPUTS = "inputs";
const std::string OUTPUTS = "outputs";
const std::string DEVICE = "device";
const std::string DBGFLAG = "dbgflag";
const std::string DEVICELOGFLAG = "devicelogFlag";
void Detector::load(libconfig::Config& config, const std::string& key) {
 
    threshold = config.lookup(key + "." + THRESHOLD);
    libconfig::Setting& inputs = config.lookup(key + "." + INPUTS);

    std::vector<std::string> in;
    for (int i = 0; i < inputs.getLength(); i++) {
      in.push_back(inputs[i]);
    }

    libconfig::Setting& outputs = config.lookup(key + "." + OUTPUTS);
    std::vector<std::string> out;
    for (int i = 0; i < outputs.getLength(); i++) {
      out.push_back(outputs[i]);
    }

    std::string device;
    if (config.lookupValue(key + "." + DEVICE, device)) {
      model.device = device;
    }
    bool dbgflag;
    if (config.lookupValue(key + "." + DBGFLAG, dbgflag)) {
      model.debugFlag = dbgflag;
    }
    bool devicelogFlag;
    if (config.lookupValue(key + "." + DEVICELOGFLAG, devicelogFlag)) {
      model.devicelogFlag = devicelogFlag;
    }
    model.load(config.lookup(key + "." + PATH), in, out);
    cv::Mat dummy(768, 1024, CV_8UC3, cv::Scalar::all(1));
    cv::Rect origin(0, 0, dummy.cols - 1, dummy.rows - 1); 
    predict(dummy, origin);
}

std::vector<cv::Rect> Detector::predict(cv::Mat& bgr, cv::Rect& rect) {
   cv::Mat mat(bgr, rect);
   tensorflow::Tensor t = convert_uint8(mat);
   std::vector<tensorflow::Tensor> result = model.predict(t);
   auto boxes = result[0].flat_inner_dims<float>();
   auto scores = result[1].flat_inner_dims<float>();
   std::vector<cv::Rect> resultMat;
   for (int i = 0; i < scores.size(); i++) {
     if (scores(i) > threshold) {
        int x1 = boxes(i, 1) * mat.cols;
        int y1 = boxes(i, 0) * mat.rows; 
        int x2 = boxes(i, 3) * mat.cols; 
        int y2 = boxes(i, 2) * mat.rows;
	      int w = x2 - x1;
        int h = y2 - y1;
        cv::Rect origrect(x1 + rect.x, y1 + rect.y, w, h);
        resultMat.push_back(origrect);
     }
   }
   return resultMat;
}

Detector::~Detector() {
}
