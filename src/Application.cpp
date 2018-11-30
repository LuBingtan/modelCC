#include "Application.h"
#include "HaiKangApplication.h"
#include "MockApplication.h"
#include "DaHuaApplication.h"
#include "CVModel.h"
#include "DB.h"
#include "time.h"

Application::Application(std::string name): Logging(name) {}

const std::string NAME = "dvr.name";
const std::string DVR_CHANNELS = "channels";
const std::string DVR_CHANNELS_REGIONS = "regions";
const std::string DVR_CHANNELS_ID = "id";
const std::string DVR_CONFIG = "config";
const std::string HAIKANG = "haikang";
const std::string MOCKHAIKANG = "mock_haikang";
const std::string MOCK = "mock";
const std::string DAHUA = "dahua";

Application* Application::createAppliction(libconfig::Config& appConfig, 
                                            libconfig::Config& dictConfig) {
    std::string name = appConfig.lookup(NAME);
    Application* app = nullptr;
    if (name == MOCK) {
       app = new MockApplication();
    } else if (name == HAIKANG){
       app = new HaiKangApplication();
    } else if (name == MOCKHAIKANG){
        app = new MockApplication();
        app -> readHaikang = true;
    } else if (name == DAHUA){
       app = new DaHuaApplication();
    }else {
        std::cout << "dvr name" + name + "not supported yet" << std::endl;
        return app;
    }
    return app;
}

bool Application::login() {
    return true;
}

bool Application::logout() {
    return true;
}

void Application::cameraShot(std::vector<int>& ids, std::string& path) {

}

bool Application::init(libconfig::Config& config) {
    
    const libconfig::Setting& channelDesc = config.lookup(DVR_CHANNELS);
    for (int i = 0; i < channelDesc.getLength(); i++) {
        unsigned int chId = channelDesc[i][DVR_CHANNELS_ID];
        if (channels.count(chId)) {
            logError("chId: " + std::to_string(chId) + " is duplicated");
            return false;
        }
        auto channel = createChannel(chId, this);
        channel -> init(channelDesc[i]);

        channel -> config = Configuration::getConfiguration(
            config.lookup(DVR_CONFIG), 
            channel -> configName);
        channel -> writer = new VideoWriter(channel -> config -> videoOrig,
                                     channel -> config -> videoDebug,
                                     channel -> config -> path,
                                     channel -> name);
        channel -> writer -> keepDays = channel -> config-> keepDays;
        channel -> db.init(config);
        
        channels[chId] = channel;
    }
    return true;
}

bool Application::start() {
    if (!login()) {
        return false;
    }
    for (auto ch: channels) {
        if (!(ch.second) -> start()) {
            return false;
        }
    }
    running = true;
    monitor = std::thread(&Application::do_monitor, this);
    return true;
}

bool Application::stop() {
    logout();
    for (auto ch: channels) {
        (ch.second) -> stop();
    }
    running = false;
    return true;
}

void Application::join() {
    for (auto ch: channels) {
        (ch.second) -> join();
    }
}

void Application::do_monitor() {
    while(running) {
        std::this_thread::sleep_for(std::chrono::seconds(15));
        for(auto kv : channels) {
            if ((kv.second) -> cur_time != (time_t) 0 ) {
                auto current = std::chrono::system_clock::from_time_t((kv.second) -> cur_time);
                auto baseTime = std::chrono::system_clock::now();
                auto delta = baseTime - current;
                if(delta > std::chrono::seconds(30)) {
                    logError("chId: " + (kv.second) -> name + " can receive no picture");
                    exit(1);
                }
            }
        }
    }
}

Application::~Application() {
    for (auto ch: channels) {
        delete (ch.second);
    }
}