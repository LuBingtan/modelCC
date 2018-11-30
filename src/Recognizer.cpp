#include <iostream>
#include <vector>
#include <map>

#include <opencv/cv.h>
#include <opencv2/imgproc/imgproc.hpp>

#include "Recognizer.h"

Recognizer::Recognizer(std::string name): Logging(name + "Recognizer"), model(name) {
}

const std::string PATH = "path";
const std::string WIDTH = "width";
const std::string HEIGHT = "height";

const std::string INPUTS = "inputs";
const std::string OUTPUTS = "outputs";
const std::string DEVICE = "device";
const std::string DBGFLAG = "dbgflag";
const std::string DEVICELOGFLAG = "devicelogFlag";
void Recognizer::load(libconfig::Config& config, const std::string& key) {
    width = config.lookup(key + "." + WIDTH);
    height = config.lookup(key + "." + HEIGHT);

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
    if(config.lookupValue(key + "." + DEVICE, device)) {
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
}

Recognizer::~Recognizer() {
}
