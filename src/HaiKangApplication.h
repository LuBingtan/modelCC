#ifndef _HAIKANG_REALAPPLICATION_H_
#define _HAIKANG_REALAPPLICATION_H_

#include <libconfig.h++>

#include "Application.h"
#include "public.h"

class HaiKangApplication: public Application {
public:
    HaiKangApplication();
    bool init(libconfig::Config&) override;
    bool login() override;
    bool logout() override;
    Channel* createChannel(unsigned int, Application*) override;
    void cameraShot(std::vector<int>& ids, std::string& path) override;
    ~HaiKangApplication() override;

    LONG lUserID = -1;

private:
    NET_DVR_IPPARACFG ipParam;
    BOOL hasInited = FALSE;
    std::string host;
    unsigned int port;
    std::string username;
    std::string password;
    unsigned int firstChannel;
    unsigned int lastChannel;
};

#endif