#ifndef COLORRECOGNIZER_H
#define COLORRECOGNIZER_H
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <map>

using namespace std;
class LicenceColorRecognizer{
public:
    LicenceColorRecognizer();
    ~LicenceColorRecognizer();
    enum LicenceColor {BLUE = 0, YELLOW = 1, GREEN = 2, BLACK = 3, WHITE = 4, GRAY = 5};
    LicenceColor recognize(cv::Mat &img, bool iscluster);
    LicenceColor recognize_hist(cv::Mat &img);
    LicenceColor recognize_cluster(cv::Mat &img);
    vector<pair<cv::Vec3b,cv::Vec3b>> color_threshold;
private:
    vector<LicenceColor> colorToRecognize;
    vector<cv::Mat> genColorImg(cv::Mat &img);
    double calValDis(cv::Mat &img_val);
    vector<int> colorHist(cv::Mat &img, vector<pair<cv::Vec3b, cv::Vec3b>> &threshold_vec);
    bool isthisColor(cv::Vec3b pixel, pair<cv::Vec3b, cv::Vec3b> threshold);
    pair<cv::Mat, cv::Mat> cluster(cv::Mat &img);
};
#endif
