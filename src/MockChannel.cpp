#include <iostream>
#include <experimental/filesystem>
#include <sstream>
#include <vector>
#include <map>
#include <jsoncpp/json/json.h>

#include <boost/locale/conversion.hpp>
#include <boost/locale/encoding.hpp>

#include <stdio.h>
#include <locale>
#include <codecvt>

#include "HCNetSDK.h"
#include "PlayM4.h"

#include "MockChannel.h"
#include "Application.h"
#include "CVModel.h"
#include "utils.h"

MockChannel::MockChannel(LONG ch, MockApplication* mapp): Channel(ch), app(mapp) {
    readHaikang = app -> readHaikang;
}

const std::string DVR_CHANNELS_FILE = "mock_videos";
const std::string DVR_CHANNELS_FPS = "mock_fps";
const std::string DVR_MOCK_DIT = "mock_dit";
const std::string DVR_MOCK_ENABLE = "mock_enable";


const std::string Comma = ",";
const std::string Colon = ":";
const int PUMP_ID = 0;
const int PUMP_STATUS = 1;
const int PUMP_TIME = 2;
const int PUMP_LICENSE = 3;

LabeledData::LabeledData(int id, int st, std::chrono::seconds t, std::string lic):pumpId(id), pumpStatus(st), time(t), license(lic){

}

std::string LabeledData::key() {
    return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(time).count());
}

std::chrono::seconds getDuration(std::string& timeString) {
    std::vector<std::string> sps = split(timeString, Colon);
    if (sps.size() == 2) {
        return std::chrono::minutes(std::stoi(sps[0])) + std::chrono::seconds(std::stoi(sps[1]));
    }
    if (sps.size() == 3) {
        return std::chrono::hours(std::stoi(sps[0])) + std::chrono::minutes(std::stoi(sps[1])) + std::chrono::seconds(std::stoi(sps[2]));
    }
    return std::chrono::seconds(0);
}

bool MockChannel::init(libconfig::Setting& config) {
    Channel::init(config);
    std::string videos, labels;
    config.lookupValue(DVR_CHANNELS_FILE, videos);
    config.lookupValue(DVR_CHANNELS_FILE, labels);
    config.lookupValue(DVR_MOCK_ENABLE, enable);
    if (!enable) {
        return true;
    }

    std::map<std::string, std::string> keyToVideo;
    for (auto & p : std::experimental::filesystem::directory_iterator(videos)) {
        if (std::experimental::filesystem::is_regular_file(p) && (p.path().extension() == ".avi" || p.path().extension() == ".mp4")) {
            keyToVideo[p.path().stem()] = p.path();
        }
    }

    std::map<std::string, std::string> keyToLabel;
    for (auto & p : std::experimental::filesystem::directory_iterator(labels)) {
        if (std::experimental::filesystem::is_regular_file(p) && p.path().extension() == ".csv") {
            keyToLabel[p.path().stem()] = p.path();
        }
    }

    for (auto pair: keyToVideo) {
        if (keyToLabel.count(pair.first) > 0) {
            auto avi = pair.second;
            auto csv = keyToLabel[pair.first];
            mockVideos.push(std::make_pair(avi, csv));
        }
    }


    int fps;
    if(config.lookupValue(DVR_CHANNELS_FPS, fps)) {
        mockFps  = fps;
    }


    std::string address;
    if(config.lookupValue(DVR_MOCK_DIT, address)) {
       	get.address = address;
    }
    return true;
}

void MockChannel::loadLabeledData(std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        logInfo(filename);
        while(std::getline(file, line)) {
            logInfo(line);
            std::vector<std::string> sps = split(trim(line), Comma);
            std::string license;
            if (sps.size() == 4) {
               license = sps[PUMP_LICENSE];
            }
            LabeledData data(std::stoi(sps[PUMP_ID]), 
                             std::stoi(sps[PUMP_STATUS]), 
                             getDuration(sps[PUMP_TIME]), 
                             license);
            mockData[data.key()].push_back(data);
        }
}

void MockChannel::post(cv::Mat& src, time_t& time) {
    Channel::post(src, time);
    auto current = std::chrono::system_clock::from_time_t(time);
    auto baseTime = std::chrono::system_clock::from_time_t(base);
    auto delta = current - baseTime;
    auto seconds = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(delta).count());
    curlQueue.add(seconds);
}

void MockChannel::cleanup() {
    pass = 0;
    fail = 0;
    mockData.clear();
    lastFrame.clear();
}
/*haikang sdk decoding*/
std::map<LONG, MockChannel*> port2channel_mock;
void CALLBACK decodeFun_haikang(LONG nPort, char * pBuf, int nSize, FRAME_INFO * pFrameInfo, void* nReserved1, int nReserved2) {
    cv::Mat yv12 = cv::Mat(pFrameInfo->nHeight * 3 / 2 , pFrameInfo->nWidth, CV_8UC1, pBuf);
    MockChannel* ch = port2channel_mock[nPort];
    if (pFrameInfo -> dwFrameNum % ch -> config -> mod == 0) {
        time_t temp_time = time(0);
        ch -> q.add(std::make_pair(yv12, temp_time));
    }
    if (ch -> config -> videoOrig) {
        ch -> origQueue.add(yv12);
    }
}
bool MockChannel::read_haikang(std::string name) {
    std::cout << "read_haikang" << std::endl;
    std::shared_ptr<char> tmp(new char[name.size() + 1]);
    strcpy(tmp.get(), name.c_str());
    unsigned int dRet;
    int nPort;
    if(!PlayM4_GetPort(&nPort)) {
        std::cout << "PlayM4_GetPort fail" << std::endl;
    }
    std::cout << "PlayM4_GetPort success" << std::endl;
    port2channel_mock[nPort] = this;
    // start play
    if (!PlayM4_OpenFile(nPort, tmp.get())) {
	    dRet = PlayM4_GetLastError(nPort);
        std::cout << "PlayM4_OpenFile fail" << dRet << std::endl;
		return false;
    }
    std::cout << "PlayM4_OpenFile success" << std::endl;
	if (!PlayM4_SetDecCallBack(nPort, decodeFun_haikang)) {
	    dRet = PlayM4_GetLastError(nPort);
        std::cout << "PlayM4_SetDecCallBack fail:" << dRet << std::endl;
        return false;
    }
    std::cout << "PlayM4_SetDecCallBack success" << std::endl;
	if (!PlayM4_Play(nPort, 0)) {
	    dRet = PlayM4_GetLastError(nPort);
        std::cout << "PlayM4_Play fail:" << dRet << std::endl;
        return false;
	}
    base = time(0);
    std::cout << "PlayM4_Play success" << std::endl;
    while(PlayM4_GetPlayedTime(nPort) < PlayM4_GetFileTime(nPort)-1) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        continue;
    }
    std::string result = " channel: " + name + 
                    " total: " + std::to_string(pass + fail) +
                    " pass: " + std::to_string(pass) + 
                    " fail: " + std::to_string(fail) +
                    " accuracy: " + std::to_string(((double)pass) / (pass + fail));
    std::cout << result << std::endl;
    // stop play
    PlayM4_Stop(nPort);
    PlayM4_CloseFile(nPort);
    PlayM4_FreePort(nPort);
    return true;
}
/**********************/
/****opencv decoding***/
bool MockChannel::read_opencv(std::string name) {
    cv::VideoCapture capture(name);
    if (!capture.isOpened()) {
        std::cout << "fail to open " << name << std::endl;
        return false;
    }
    std::cout << "success to open " << name << std::endl;
    int realFps = capture.get(CV_CAP_PROP_FPS);
    int interval = 1000 * 1000 / mockFps;
    int timeCompensation = 0;
    long long count = 0;

    base = time(0);
    auto baseTime = std::chrono::system_clock::from_time_t(base);
    
    while (running) {
        cv::Mat frame;
        if (!capture.read(frame)) {
            break;
        }

        auto start = std::chrono::system_clock::now();
        count++;
        if (count % config -> mod == 0) {
            long long frameCount = count / config -> mod;
            auto delta = frameCount * config -> mod / realFps;
            
            auto adjected = std::chrono::system_clock::to_time_t(std::chrono::seconds(delta) + baseTime);
            q.add(std::make_pair(frame, adjected));
        }
        if (config -> videoOrig) {
            origQueue.add(frame);
        }
        auto end = std::chrono::system_clock::now();
        auto diff = end - start;
        auto time = std::chrono::duration_cast<std::chrono::microseconds>(diff).count();
        std::this_thread::sleep_for(std::chrono::microseconds(interval + timeCompensation - time));

        auto afterSleep = std::chrono::system_clock::now();
        auto diffIncludeSleep = afterSleep - start;
        auto timeWithSleep = std::chrono::duration_cast<std::chrono::microseconds>(diffIncludeSleep).count();
        timeCompensation = interval + timeCompensation - timeWithSleep;
    }
    while (q.size() != 0) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::string result = " channel: " + name + 
                        " total: " + std::to_string(pass + fail) +
                        " pass: " + std::to_string(pass) + 
                        " fail: " + std::to_string(fail) +
                        " accuracy: " + std::to_string(((double)pass) / (pass + fail));
    std::cout << result << std::endl;
    capture.release();
    return true;
}
/**********************/
void MockChannel::read() {
    unsigned long long  index = 0;
    while (mockVideos.size() > 0 && running) {
        cleanup();

        auto videoAndLabel = mockVideos.front();
        auto avi = videoAndLabel.first;
        auto csv = videoAndLabel.second;
        mockVideos.pop();
        mockVideos.push(videoAndLabel);
        logDebug("DEBUGINFO: video number is " + std::to_string(index++));
        std::cout << avi << " " << csv << std::endl;
        loadLabeledData(csv); 
        std::cout << "success to load " << csv << std::endl;
        if(readHaikang) {
            read_haikang(avi);
        } else {
            read_opencv(avi);
        }
    }
    logDebug("read thread exit");
}

cv::Mat& MockChannel::convert(cv::Mat& src, cv::Mat& dist) {
    if(readHaikang) {
        cv::cvtColor(src, dist, CV_YUV2BGR_YV12);
        return dist;
    } else {
        return src;
    }
}

const std::string jsonKey = "sLicense";

std::string default_console = "\033[0m";
inline std::string color(int i) {
    return "\033[0;" + std::to_string(i) + "m";
}

inline void printColor(int c, std::string str) {
    std::cout << color(c) << str << default_console <<std::endl;
}

const int RED = 31;
const int GREEN = 32;
const int YELLOW = 33;

void MockChannel::curlRun() {
    while (running) {
        auto time = curlQueue.remove();
        if (mockData.count(time) > 0) {
            for (auto data: mockData[time]) {
                std::string json = get.get(std::to_string(data.pumpId), std::to_string(data.pumpStatus));
                std::stringstream ss(json);
                Json::Value root;
                ss >> root;
                const Json::Value sLicense = root[jsonKey];
                std::string utf8License = boost::locale::conv::between(sLicense.asString(), "UTF-8", "GBK" );  
                int timeSec = std::stoi(time);
                std::string result = " channel: " + name +
                            " byPumpID: " + std::to_string(data.pumpId) + 
                            " byPumpStatus: " + std::to_string(data.pumpStatus) + 
                            " time: " + std::to_string(timeSec / 3600) + ":" + std::to_string(timeSec % 3600 / 60) + ":" + std::to_string(timeSec % 3600 % 60) +
                            " expected: " + data.license +
                            " actual: " + utf8License;
                if(data.pumpStatus == 3){
                    if (utf8License == data.license) {
                        printColor(GREEN, " [PASS] " + result);
                        pass++;
                    } else {
                        printColor(RED, " [FAIL] " + result);
                        fail++;
                    }
                }
            }
        }
    }
}

bool MockChannel::start() {
    if (enable) {
        Channel::start();
    
        readThread = std::thread(&MockChannel::read, this);

        curlThread = std::thread(&MockChannel::curlRun, this);
    }

    return true;
}

void MockChannel::join() {
    if (enable) {
        readThread.join();
    }
}

MockChannel::~MockChannel() {

    if (writer) {
        delete writer;
    }
}
