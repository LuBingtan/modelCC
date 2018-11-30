#ifndef CPP_HTTPGET_H
#define CPP_HTTPGET_H

#include <string>
#include <curl/curl.h>

class HttpGet {
public:

    HttpGet();

    std::string get(std::string pumpId, std::string pumpStatus);

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

    ~HttpGet();
    std::string address = "localhost:8181";
private:
    CURL *curl;
};


#endif //CPP_HTTPGET_H
