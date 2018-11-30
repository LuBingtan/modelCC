#ifndef _DAHUA_REALAPPLICATION_H_
#define _DAHUA_REALAPPLICATION_H_

#include <libconfig.h++>

#include "Application.h"


class DaHuaApplication: public Application {
public:
    DaHuaApplication();
    bool init(libconfig::Config&) override;
    bool login() override;
    bool logout() override;
    Channel* createChannel(unsigned int, Application*) override;
    ~DaHuaApplication() override;
    void cameraShot(std::vector<int>& ids, std::string& path) override;
    LONG lUserID = 0;
private:
    bool hasInited = false;
    std::string host;
    unsigned int port = 0;
    std::string username;
    std::string password;
    
    std::vector<unsigned int> channel_id;
    long play_port;
};

#endif