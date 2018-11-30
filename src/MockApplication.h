#ifndef _MOCKAPPLICATION_H_
#define _MOCKAPPLICATION_H_

#include "Application.h"

class MockApplication: public Application {
public:
    MockApplication();
    ~MockApplication() override;
    Channel* createChannel(unsigned int chId, Application* app) override;
};

#endif