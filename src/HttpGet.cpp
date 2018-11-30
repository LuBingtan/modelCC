#include "HttpGet.h"

HttpGet::HttpGet() {
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    
}

HttpGet::~HttpGet() {
    if(curl) {
        curl_easy_cleanup(curl);
    }
}

std::string HttpGet::get(std::string pumpId, std::string pumpStatus) {
    // CURLcode res;
    std::string result;
    std::string url = "http://" + address + "/Station/dit/getCarInfo?byPumpID=" + pumpId + ""
            "&byPumpStatus=" + pumpStatus;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
        curl_easy_perform(curl);
    }
    return result;
}

size_t HttpGet::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string *) userp)->append((char *) contents, size * nmemb);
    return size * nmemb;
}
