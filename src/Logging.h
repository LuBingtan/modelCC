#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>

class Logging {
public:
    Logging(std::string name);
    void logInfo(const std::string& msg);
    void logDebug(const std::string& msg);
    void logWarn(const std::string& msg);
    void logError(const std::string& msg);
    virtual ~Logging();

private:
    log4cpp::Category& c = log4cpp::Category::getRoot();
    std::string prefix;
};

#endif