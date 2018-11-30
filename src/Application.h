#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <vector>
#include <libconfig.h++>

#include "Channel.h"
#include "Logging.h"

class Application: public Logging{
public:

    static Application* createAppliction(libconfig::Config&, libconfig::Config&);

    Application(std::string name);
    virtual bool init(libconfig::Config&);
    virtual bool start();
    virtual void join();
    virtual bool login();
    virtual bool logout();
    virtual bool stop();
    virtual Channel* createChannel(unsigned int, Application*) = 0;
    virtual void cameraShot(std::vector<int>& ids, std::string& path);

    virtual ~Application();
    std::map<unsigned int, Channel *> channels;
    std::thread monitor;
    void do_monitor();
    bool readHaikang = false;
    std::atomic<bool> running;
};

#endif