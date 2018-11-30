#include <iostream>
#include <vector>
#include <map>
#include <stdlib.h>
#include <unistd.h>
#include "Model.h"
#include "utils.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/util/device_name_utils.h"
#include "tensorflow/core/protobuf/debug.pb.h"
#include "tensorflow/core/framework/graph.pb.h"
using namespace tensorflow;
bool Model::dbgFirstTime = false;
int Model::modelNum = 0;
Model::Model(std::string name): Logging(name + "Model"), modelName(name),
    debugFlag(false), devicelogFlag(false), readyDbg(false),
    dbgTime(0), preidctToolongTimes(0) {
        modelNum++;
        logDebug("modelNum is "+std::to_string(modelNum));
}

Model::~Model(){}

int Model::load(const std::string path, std::vector<std::string>& in, std::vector<std::string>& out) {
        //Read the pb file into the grapgdef member

    tensorflow::SessionOptions opts;
    // opts.config.set_log_device_placement(true);

    opts.config.mutable_gpu_options()->set_allow_growth(true);
    // opts.config.mutable_gpu_options()->set_per_process_gpu_memory_fraction(0.2);
    opts.config.set_log_device_placement(devicelogFlag);
    tensorflow::Status status = NewSession(opts, &session);
    if (!status.ok()) {
        std::cout << status.ToString() << "\n";
        return -1;
    }

    tensorflow::Status status_load = ReadBinaryProto(Env::Default(), path, &graphdef);
    if (!status_load.ok()) {
        std::cout << "ERROR: Loading model failed..." << path << std::endl;
        std::cout << status_load.ToString() << "\n";
        return -1;
    }

    if (device != "") {
        tensorflow::graph::SetDefaultDevice(device, &graphdef);
        logInfo("SetDefaultDevice: " + device);
    }
    //print each node device @lubingtan
    int node_count = graphdef.node_size();
    for (int i = 0; i < node_count; i++) {
        auto node = graphdef.mutable_node(i);
        std::cout<< node->name() <<" device is " << node->device() << std::endl;
    }
    // Add the graph to the session
    tensorflow::Status status_create = session->Create(graphdef);
    //add tfdbg watch
    ops=tensorflow::RunOptions();
    tensorflow::DebugOptions* dp=ops.mutable_debug_options();
    //debug option set
    for(int i=0;i<node_count;i++) {
        auto node = graphdef.mutable_node(i);
        DebugTensorWatch* watch = dp->add_debug_tensor_watch_opts();
        watch->set_node_name(node->name());
        std::cout<<node->name()<<std::endl;
        watch->set_output_slot(0);
        watch->add_debug_ops("DebugIdentity");
        watch->add_debug_urls("file://../"+modelName);
    }
    //
    if (!status_create.ok()) {
        std::cout << "ERROR: Creating graph in session failed..." << status_create.ToString() << std::endl;
        return -1;
    } std::cout << status_load.ToString() << "\n";

    _inputNames = in;
    _outputNames = out;

    return 0;
}
void removeDir(std::string path) {
    // if folder exists delete
    if(access(path.c_str(), F_OK) == 0) {
        std::string cmdStr = "rm -rf " + path;
        system(cmdStr.c_str());
    }
}
bool Model::predict_tfdbg(tensorflow::Tensor& input, std::vector<Tensor>* output) {
    //线程加锁
    try {
        std::unique_lock<std::mutex> locker(mutex, std::defer_lock);
        if(locker.try_lock()) {
            if(!modelName.empty()) {
                std::string path = "../" + modelName;
                removeDir(path);
            }
            session->Run(ops, {{_inputNames[0], input}}, _outputNames, {}, output, &runMetadata);
            locker.unlock();
            return true;
        } else {
            return false;
        }
        // "unique_lock"析构时mutex自动解锁
    }
    catch (std::logic_error& error) {
        logDebug("Predict_tfdbg ERROR: exception caught" + string(error.what()));
        return false;
    }
}
std::vector<tensorflow::Tensor> Model::predict(tensorflow::Tensor& input) {
    // 配置文件中dbgflag=true时输出debugger信息
    // 第一次predict时输出一次
    // 每次predict时间过长"preidctToolongTimes"加1，每隔50次过长输出一次debugger信息
    // 如果"predict_tfdbg"失败，继续正常的predict
    std::vector<tensorflow::Tensor> outputs;
    try {
        bool debuggerCreate = false;;
        if(debugFlag == true) {
            if(!dbgFirstTime) {
                dbgFirstTime = true;
                debuggerCreate = predict_tfdbg(input, &outputs);
                if(debuggerCreate) {
                    logDebug("First time TFDBG_Testing Done.");
                }
            } else {
                if(readyDbg && preidctToolongTimes%50 == 0) {
                    debuggerCreate = predict_tfdbg(input, &outputs);
                    if(debuggerCreate) {
                        dbgTime++;
                        logDebug("TFDBG_Testing time " + std::to_string(dbgTime));
                    }
                }
            }
        }
        readyDbg = false;
        if(!debuggerCreate) {
            auto start = std::chrono::steady_clock::now();
            tensorflow::Status status = session->Run({{_inputNames[0], input}}, _outputNames, {}, &outputs);
            auto end = std::chrono::steady_clock::now();
            auto diff = end - start;
            auto count =  std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(diff).count());
            logDebug("predict time: " + count);
            if(std::atoi(count.c_str())>2000) {
                readyDbg=true;
                preidctToolongTimes++;
                logDebug("Predict time too_long! Time: " + std::to_string(preidctToolongTimes));
            } else {
                readyDbg=false;
            }
            if (!status.ok()) {
                std::cout << "ERROR: prediction failed..." << status.ToString() << std::endl;
            }
        }
    }
        catch (std::logic_error& error) {
         logDebug("exception caught" + string(error.what()));
    }
    return outputs;
}
