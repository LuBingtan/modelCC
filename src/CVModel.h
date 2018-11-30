#ifndef _CVMODEL_H_
#define _CVMODEL_H_
#include <opencv/cv.h>
#include <libconfig.h++>

#include "Detector.h"
#include "Recognizer.h"
#include "Target.h"
#include "Cmt.h"
#include "LicenceColorRecognizer.h"
class CVModel {
public:
    static CVModel & GetInstance() {  
        static CVModel instance;
        return instance;  
    }
    ~CVModel();
    int init(libconfig::Config& config);
    int loadDict(libconfig::Config& config);
    Detector vehicle;
    Detector license;
    Recognizer ocr;
    Recognizer colorMakeType;
    Recognizer digital;
    std::vector<std::string> color;
    std::vector<std::string> make;
    std::vector<std::string> type;
    std::pair<std::string, cv::Rect> predictLicense(cv::Rect& rect, cv::Mat& src);
    CMT predictColorMakeType(cv::Rect& r, cv::Mat& src);
    std::string getTime(cv::Mat& src);
    LicenceColorRecognizer color_recognizer;
    std::string predictColor(cv::Rect& rect, cv::Mat& src);
private:
    CVModel();
    CVModel(const CVModel &);  
    CVModel & operator = (const CVModel &);  

};

#endif
