#include "Logging.h"

Logging::Logging(std::string name):prefix(name) {

}

void Logging::logInfo(const std::string& msg) {
    c.info(prefix + ":" + msg);
}

void Logging::logDebug(const std::string& msg) {
    c.debug(prefix + ":" + msg);
}

void Logging::logWarn(const std::string& msg) {
    c.warn(prefix + ":" + msg);
}

void Logging::logError(const std::string& msg) {
    c.error(prefix + ":" + msg);
}

Logging::~Logging() {

}