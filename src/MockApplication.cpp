#include "MockApplication.h"
#include "MockChannel.h"

MockApplication::MockApplication(): Application("MockApplication") {}

MockApplication::~MockApplication() {}

Channel* MockApplication::createChannel(unsigned int chId, Application* app) {
    return new MockChannel(chId, static_cast<MockApplication*>(app));
}