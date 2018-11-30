#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <string>
#include <map>
#include <libconfig.h++>

#include "Target.h"
#include "Logging.h"


class Config: public Logging {
public:
    Config(std::string, int);
    virtual ~Config();
    virtual std::pair<bool, Target> lookup(cv::Mat& src, 
            Target& t,
            std::map<std::string, Target>& lastFrame) = 0;
    std::string name;
    int stayCount;
};

class OcrConfig: public Config {
public:
    OcrConfig(std::string name, int missCount, int stayCount);
    ~OcrConfig() override;
    std::pair<bool, Target> lookup(cv::Mat& src, 
            Target& t, 
            std::map<std::string, Target>& lastFrame) override;
private:
    int missCount;
};

class LocationConfig: public Config {
public:
    LocationConfig(std::string n, int thres, int stayCount);
    ~LocationConfig() override;
    std::pair<bool, Target> lookup(cv::Mat& src, 
            Target& t,
            std::map<std::string, Target>& lastFrame) override;
private:
    int threshold;
};

class CmtConfig: public Config {
public:
    CmtConfig(std::string name, int matchCount, int stayCount);
    ~CmtConfig() override;
    std::pair<bool, Target> lookup(cv::Mat& src, 
            Target& t,
            std::map<std::string, Target>& lastFrame) override;
private:
    int matchCount;
};

class NewVerifyConfig: public Config {
public:
    NewVerifyConfig(std::string name, bool allowEmpty);
    ~NewVerifyConfig() override;
    std::pair<bool, Target> lookup(cv::Mat& src, 
            Target& t,
            std::map<std::string, Target>& lastFrame) override;
    bool allowEmpty;
};

class Configuration: public Logging {
public:
    static Configuration* getConfiguration(libconfig::Setting& setting, std::string& configName);
    std::pair<bool, Target> lookup(cv::Mat& src, 
            Target& t,
            std::map<std::string, Target>& lastFrame);
    ~Configuration();
    std::vector<Config *> configs;
    int stayCount;
    int outCount;
    int mod;
    bool videoOrig;
    bool videoDebug;
    bool onGPU = false;
    std::string path;
    int keepDays;
private:
    Configuration();
};

#endif