#ifndef __APPLE__


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <cstdio>
#include <libconfig.h++>
#include <log4cpp/PropertyConfigurator.hh>
#include "Application.h"

Application* app;

const std::string OPTION_SERVER = "server";
const std::string OPTION_CMT = "cmt";
const std::string OPTION_LOG4CPP = "log4cpp";
const std::string OPTION_OUTPUT = "output";
const std::string OPTION_HELP = "output";

int main(int argc, char* argv[]) {

    std::string confPath = "../../conf/server.conf";
    std::string logConfPath = "../../conf/log4cpp.properites";
    std::string dictPath = "../../conf/cmt.conf";
    std::string outPath = "./";

    bool testOnly = false;
    std::vector<int> ids;
    std::string indexs;

    int opt;
    while((opt = getopt(argc, argv, "s:l:c:o:ti:")) != -1) {
        switch(opt) {
        case 's':
            confPath = std::string(optarg);
            break;
        case 'l':
            logConfPath = std::string(optarg);
            break;
        case 'c':
            dictPath = std::string(optarg);
            break;
        case 'o':
            outPath = std::string(optarg);
            break;
        case 't':
            testOnly = true;
            break;
        case 'i':
            indexs = std::string(optarg);
            {
                std::string sp = ",";
                std::vector<std::string> idsVec = split(indexs, sp);
                for (auto i: idsVec) {
                    ids.push_back(std::stoi(i));
                }
            }
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-s server.conf] [-l log4cpp.properites] [-c cmt.conf] [-o image-output-path] [-t] [-i 1,2,3]\n", argv[0]); 
            exit(EXIT_FAILURE);
        }
    }  

    libconfig::Config config;
    libconfig::Config dict;

    try {
        config.readFile(confPath.c_str());
        dict.readFile(dictPath.c_str());
    }
    catch(libconfig::FileIOException &e) {
        std::cout << "read file [ "  << confPath 
                << " ] FileIOException:" << std::endl;
        return -1;
    }
    catch(libconfig::ParseException &e) {
        std::cout << "read file [ " << confPath 
            << " ] ParaseException:" << e.getError() 
            << ",line:" << e.getLine() << std::endl;
        return -1;
    }
    catch(...) {
        std::cout << "read file [" << confPath 
            << " ] unknown exception" << std::endl;
        return -1;
    }

    log4cpp::PropertyConfigurator::configure(logConfPath);

    app = Application::createAppliction(config, dict);
    if (app && app -> init(config)) {
        bool success = app -> login();
        if (testOnly) {
            if (success) {
                app -> logout();
                return 0;
            } else {
                return 1;
            }
        }
        if (success) {
            app -> cameraShot(ids, outPath);
            app -> logout();
        }
    }
    return 0;
}

#endif
