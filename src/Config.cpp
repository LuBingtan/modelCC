#include <string>
#include <functional>
#include <map>

#include "Config.h"
#include "CVModel.h"

const std::string CONFIG = "config";
const std::string NAME = "name";
const std::string MOD = "mod";
const std::string STAY_COUNT = "stay_count";
const std::string OUT_COUNT = "out_count";
const std::string MODEL = "model";
const std::string VIDEO = "video";
const std::string VIDEO_ORIG = "write_original";
const std::string VIDEO_DEBUG = "write_debug";
const std::string VIDEO_PATH = "path";
const std::string VIDEO_KEEP = "keep";

const std::string ON_GPU = "on_gpu";
const std::string MODEL_NAME_OCR = "ocr";
const std::string MODEL_NAME_CMT = "cmt";
const std::string MODEL_NAME_LOCATION = "location";
const std::string MODEL_NAME_VERIFY = "verify";

Config::Config(std::string n, int s): Logging(n), name(n), stayCount(s) {}
Config::~Config() {}

// ocr
const std::string MISS_COUNT = "miss_count";

Config* createOcrConfig(libconfig::Setting& setting, libconfig::Setting& config) {
    int stayCount = config[STAY_COUNT];
    int miss = setting[MISS_COUNT];
    std::string name = setting[NAME];
    return new OcrConfig(name, miss, stayCount);
}
std::pair<bool, int> licenseMatch(std::string l1, std::string l2, int count) {
    if (l1.size() == l2.size()) {
        int mismatchCount = 0;
        for (unsigned int i = 0; i < l1.size(); i++) {
            if (l1[i] != l2[i]) {
                mismatchCount++;
            }
        }
        if (mismatchCount > count) {
            return std::make_pair(false, mismatchCount);
        } else {
            return std::make_pair(true, mismatchCount);;
        }
    } else {
        return std::make_pair(false, -1);
    }
}
OcrConfig::OcrConfig(std::string n, int miss, int stayCount): Config(n, stayCount), missCount(miss) {}
OcrConfig::~OcrConfig() {}
std::pair<bool, Target> OcrConfig::lookup(cv::Mat& src, 
    Target& target, 
    std::map<std::string, Target>& lastFrame) {
    auto ocrAndRect = CVModel::GetInstance().predictLicense(target.rect, src);
    Target sameTarget;
    if (ocrAndRect.first.empty()) {
        logDebug("ocr result is empty " + target.id);
        return std::make_pair(false, target);
    } else {
        logDebug("ocr result: " + ocrAndRect.first);
        int maxMatch = -1;
        for (auto kv: lastFrame) {
            Target& t = kv.second;
            if (t.state == OUT) {
                continue;
            }
            auto match = licenseMatch(ocrAndRect.first, t.maxOccurrences(t.license), missCount);
            if (match.first && match.second > maxMatch) {
                maxMatch = match.second;
                sameTarget = t;
            }
        }
        if (maxMatch < 0) {
            target.updateOcr(ocrAndRect);
            logDebug("not find ocrTarget, return original target " + target.toString());
            return std::make_pair(false, target);
        } else {
            logDebug("find ocrTarget:" + sameTarget.id + 
                    " with match: " + std::to_string(maxMatch));
            sameTarget.updateOcr(ocrAndRect);
            sameTarget.stayOnce(target.rect, stayCount, src);        
            return std::make_pair(true, sameTarget);
        }
    }
}


// center point (x, y)
inline double dist(const cv::Point& a, const cv::Point& b) {
    auto diff = a - b;
    return cv::sqrt(diff.x * diff.x + diff.y * diff.y);
}

const std::string THRESHOLD = "threshold";
Config* createLocationConfig(libconfig::Setting& setting, libconfig::Setting& config) {
    int stayCount = config[STAY_COUNT];
    int threshold = setting[THRESHOLD];
    std::string name = setting[NAME];
    return new LocationConfig(name, threshold, stayCount);
}
LocationConfig::LocationConfig(std::string n, int thres, int stayCount): Config(n, stayCount), threshold(thres) {}
LocationConfig::~LocationConfig() {}
std::pair<bool, Target> LocationConfig::lookup(cv::Mat& src, 
                                        Target& target,
                                        std::map<std::string, Target>& lastFrame) {
    
    double minDist = 10000000.0;
    bool find = false;
    Target minTarget;
            
    for (auto kv: lastFrame) {
        Target& t = kv.second;
        if (t.state == OUT) {
            continue;
        }
        auto d = dist(target.center(), t.center());
        if (d < threshold && d < minDist) {
            minDist = d;
            minTarget = t;
            find = true;
        }
    }
    if (!find) {
        logDebug("not find LocationMinTarget, return original target " + target.id);
        return std::make_pair(false, target);
    } else {
        logDebug("find LocationMinTarget:" + minTarget.id +
                " with distance: " + std::to_string(minDist));
        minTarget.stayOnce(target.rect, stayCount, src);  
        return std::make_pair(true, minTarget);
    }
}

// color make type
const std::string MATCH_COUNT = "match_count";
Config* createCmtConfig(libconfig::Setting& setting, libconfig::Setting& config) {
    int stayCount = config[STAY_COUNT];
    int matchCount = setting[MATCH_COUNT];
    std::string name = setting[NAME];
    return new CmtConfig(name, matchCount, stayCount);
}
CmtConfig::CmtConfig(std::string n, int mc, int stayCount): Config(n, stayCount), matchCount(mc) {}
CmtConfig::~CmtConfig() {}
std::pair<bool, Target> CmtConfig::lookup(cv::Mat& src, 
                                        Target& target,
                                        std::map<std::string, Target>& lastFrame) {
    auto cmt = CVModel::GetInstance().predictColorMakeType(target.rect, src);
    logDebug(cmt.toString());
    Target sameTarget;
    int maxMatch = -1;
    for (auto kv: lastFrame) {
        Target& t = kv.second;
        if (t.state == OUT) {
            continue;
        }
        bool colorEq = (cmt.color == t.maxOccurrences(t.color));
        bool makeEq = (cmt.make == t.maxOccurrences(t.make));
        bool typeEq = (cmt.type == t.maxOccurrences(t.type));
        int match = (colorEq ? 1 : 0) + (makeEq ? 1 : 0) + (typeEq ? 1 : 0);
        if (match >= matchCount && match > maxMatch) {
            maxMatch = match;
            sameTarget = t;
        }
    }
    if (maxMatch < 0) {
        target.addCmt(cmt);
        logDebug("not find CMTTarget, return original target " + target.toString());
        return std::make_pair(false, target);
    } else {
        logDebug("find CMTTarget:" + sameTarget.id + 
                 " with match: " + std::to_string(maxMatch));
        sameTarget.addCmt(cmt);
        sameTarget.stayOnce(target.rect, stayCount, src);
        return std::make_pair(true, sameTarget);
    }
}

const std::string ALLOW_EMPTY = "allow_empty";
Config* createVerifyConfig(libconfig::Setting& setting, libconfig::Setting& config) {
    bool allowEmpty = false;
    config.lookupValue(ALLOW_EMPTY, allowEmpty);
    return new NewVerifyConfig(MODEL_NAME_VERIFY, allowEmpty);
}
NewVerifyConfig::NewVerifyConfig(std::string n, bool allow): Config(n, 0), allowEmpty(allow){
    logInfo("allowEmpty: " + std::to_string(allowEmpty));
}
NewVerifyConfig::~NewVerifyConfig() {}
std::pair<bool, Target> NewVerifyConfig::lookup(cv::Mat& src, 
    Target& target,
    std::map<std::string, Target>& lastFrame) {
    if (target.license.empty()) {
        logDebug("target verify fail, license is empty: " + target.toString());
        return std::make_pair(allowEmpty, target);
    } else {
        return std::make_pair(true, target);
    }
}

std::map<std::string, std::function<Config*(libconfig::Setting&, libconfig::Setting&)> > createMap = {
    {MODEL_NAME_OCR, createOcrConfig}, 
    {MODEL_NAME_CMT, createCmtConfig}, 
    {MODEL_NAME_LOCATION, createLocationConfig},
    {MODEL_NAME_VERIFY, createVerifyConfig}
};

Configuration* Configuration::getConfiguration(libconfig::Setting& setting, std::string& configName) {
    for(int i = 0; i < setting.getLength(); i++) {

        std::string name = setting[i][NAME];
        if(name == configName) {
            auto configuration = new Configuration();

            if (setting[i].exists(VIDEO)) {
                configuration -> videoOrig = setting[i][VIDEO][VIDEO_ORIG];
                configuration -> videoDebug = setting[i][VIDEO][VIDEO_DEBUG];
                std::string p = setting[i][VIDEO][VIDEO_PATH];
                configuration -> path = p;
                configuration -> keepDays = setting[i][VIDEO][VIDEO_KEEP];
            }
            configuration -> mod = setting[i][MOD];
            configuration -> stayCount = setting[i][STAY_COUNT];
            configuration -> outCount = setting[i][OUT_COUNT];
            configuration -> onGPU = setting[i][ON_GPU];

            libconfig::Setting& models = setting[i][MODEL];
            for (int m = 0; m < models.getLength(); m++) {
                std::string modelName = models[m][NAME];
                configuration -> configs.push_back(createMap[modelName](models[m], setting[i]));
            }
            libconfig::Setting& dummy = setting[i];
            configuration -> configs.push_back(createMap[MODEL_NAME_VERIFY](dummy, dummy));
            return configuration;
        }
    }
    std::cout << "can't find config: " + configName << std::endl;
    return nullptr;
}
std::pair<bool, Target> Configuration::lookup(cv::Mat& src, 
    Target& t,
    std::map<std::string, Target>& lastFrame) {

        std::pair<bool, Target> result = std::make_pair(false, t);
        for (auto c: configs) {
            result = c -> lookup(src, result.second, lastFrame);
            if (result.first) {
                return result;
            }
        }
        return result;
}

Configuration::Configuration(): Logging("Configuration") {}

Configuration::~Configuration() {
    for (auto c: configs) {
        if(c) {
            delete c;
        }
    }
}