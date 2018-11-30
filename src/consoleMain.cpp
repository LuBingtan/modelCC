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
#include "CVModel.h"

Application* app;
void fnExit() {
    if (app) {
        app -> stop();
        delete app;
    }
}

int main(int argc, char* argv[]) {

    // atexit (fnExit);
    
    std::string confPath = "../../conf/server.conf";
    std::string logConfPath = "../../conf/log4cpp.properites";
    std::string dictPath = "../../conf/cmt.conf";

    int opt;
    while((opt = getopt(argc, argv, "s:l:c:")) != -1) {
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
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-s server.conf] [-l log4cpp.properites] [-c cmt.conf]\n", argv[0]); 
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
        std::cout << "cv model init start" << std::endl;
        CVModel::GetInstance().init(config);
        CVModel::GetInstance().loadDict(dict);
        std::cout << "cv model init done" << std::endl;
        if(app -> start()) {
            app -> join();
            app -> stop();
        }
    }
    return 0;
}

#endif
