#ifndef _MODEL_H_
#define _MODEL_H_

#include <map>
#include <thread>         // std::thread
#include <mutex>          // std::mutex, std::lock_guard

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/protobuf/config.pb.h"
#include "Logging.h"

class Model: public Logging {
public:
    Model(std::string name);

    ~Model();

    int load(const std::string, std::vector<std::string>& in, std::vector<std::string>& out);    //Load graph file and new session

    std::vector<tensorflow::Tensor> predict(tensorflow::Tensor&);
    bool predict_tfdbg(tensorflow::Tensor&, std::vector<tensorflow::Tensor>*);
    std::vector<std::string> _inputNames;

    std::vector<std::string> _outputNames;

    std::string device;

    bool debugFlag;
    bool devicelogFlag;
private:
    tensorflow::GraphDef graphdef;

    tensorflow::Session* session;

    std::string modelName;
    
    tensorflow::RunOptions ops;
    tensorflow::RunMetadata runMetadata;
    bool readyDbg;
    int dbgTime;
    int preidctToolongTimes;
    static bool dbgFirstTime;
    static int modelNum;
    std::mutex mutex;
};
#endif