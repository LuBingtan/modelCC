#include "LicenceColorRecognizer.h"
#include <algorithm>
#include <iterator>
#include <numeric> 
#include <opencv2/ml/ml.hpp>
#include <sys/time.h> 
const int White_Sat_Threshold = 30;
const int White_Val_Threshold = 221;
const int Gray_Sat_Threshold = 43;
const int Black_Sat_Threshold = 10;
const int Black_Val_Threshold = 150;

const cv::Vec3b white_low_threshold(0,0,White_Val_Threshold);
const cv::Vec3b white_high_threshold(180,White_Sat_Threshold,255);

const cv::Vec3b black_low_threshold(0,0,0);
const cv::Vec3b black_high_threshold(180,Black_Sat_Threshold,Black_Val_Threshold);

const cv::Vec3b gray_low_threshold(0,0,Black_Val_Threshold);
const cv::Vec3b gray_high_threshold(180,Gray_Sat_Threshold,White_Val_Threshold);

const cv::Vec3b blue_low_threshold(85,30,20);
const cv::Vec3b blue_high_threshold(150,255,255);

const cv::Vec3b yellow_low_threshold(8,30,20);
const cv::Vec3b yellow_high_threshold(43,255,255);

const cv::Vec3b green_low_threshold(44,30,20);
const cv::Vec3b green_high_threshold(75,255,255);

LicenceColorRecognizer::LicenceColorRecognizer()
{
    //initialize color and color space range
    colorToRecognize.push_back(BLUE);
    color_threshold.push_back(pair<cv::Vec3b, cv::Vec3b>(blue_low_threshold,blue_high_threshold));
    colorToRecognize.push_back(YELLOW);
    color_threshold.push_back(pair<cv::Vec3b, cv::Vec3b>(yellow_low_threshold,yellow_high_threshold));
    colorToRecognize.push_back(GREEN);
    color_threshold.push_back(pair<cv::Vec3b, cv::Vec3b>(green_low_threshold,green_high_threshold));

    colorToRecognize.push_back(WHITE);
    color_threshold.push_back(pair<cv::Vec3b, cv::Vec3b>(white_low_threshold,white_high_threshold));
    colorToRecognize.push_back(GRAY);
    color_threshold.push_back(pair<cv::Vec3b, cv::Vec3b>(gray_low_threshold,white_high_threshold));
    colorToRecognize.push_back(BLACK);
    color_threshold.push_back(pair<cv::Vec3b, cv::Vec3b>(black_low_threshold,black_high_threshold));
}
LicenceColorRecognizer::~LicenceColorRecognizer()
{}
LicenceColorRecognizer::LicenceColor LicenceColorRecognizer::recognize(cv::Mat &img, bool iscluster)
{
    //iscluster: use cluster method or not
    LicenceColor rst;
    if(iscluster)
        rst = recognize_cluster(img);
    else
        rst = recognize_hist(img);
    return rst;
}
LicenceColorRecognizer::LicenceColor LicenceColorRecognizer::recognize_hist(cv::Mat &img_input)
{
    //recognize color by using statistics histogram
    LicenceColor rst = GRAY;
    cv::Mat img_HSV;
    cv::cvtColor(img_input,img_HSV,cv::COLOR_BGR2HSV);
    vector<int> hist = colorHist(img_HSV, color_threshold);
    int loc_min=distance(begin(hist),max_element(begin(hist),end(hist)));
    rst = colorToRecognize.at(loc_min);
    if(rst == BLACK || rst == WHITE || rst == GRAY)
    {
        int i=distance(begin(hist),max_element(begin(hist),end(hist)-3));
        if(hist[i] >= accumulate(hist.begin(),hist.end(),0) / 6)
            return colorToRecognize.at(i);
    }
    if(rst == GRAY)
    {
        vector<cv::Mat> img_split;
        cv::split(img_HSV,img_split);
        double dis_val = calValDis(img_split[2]);
        if(dis_val > 128)
            return WHITE;
        else
            return BLACK;
    }
    return rst;
}
LicenceColorRecognizer::LicenceColor LicenceColorRecognizer::recognize_cluster(cv::Mat &img)
{
    //recognize color by using Kmeans Cluster
    LicenceColor rst;
    pair<cv::Mat, cv::Mat> cluster_rst = cluster(img);
    int pixel_num = 0;
    cv::Vec3b pixel;
    for(int i = 0; i < cluster_rst.first.rows; i++)
    {
        if(cluster_rst.first.at<float>(i,0) > pixel_num)
        {
            pixel_num = cluster_rst.first.at<float>(i,0);
            pixel = cv::Vec3b(cluster_rst.second.at<float>(i,0), cluster_rst.second.at<float>(i,1), cluster_rst.second.at<float>(i,2));
            for(int j = 0; j < color_threshold.size(); j++)
            {
                if(isthisColor(pixel,color_threshold[j]))
                {
                    
                    rst = colorToRecognize[j];
                    break;
                }
            }
        }
    }
    if(rst == GRAY)
    {
        if(pixel[2] < 128)
            rst = BLACK;
        else
            rst = WHITE;
    }
    return rst;
}
double LicenceColorRecognizer::calValDis(cv::Mat &img_val)
{
    //calculte distance of value channel
    assert(img_val.channels() == 1);
    double dis = cv::mean(img_val).val[0];
    return dis;
}
vector<int> LicenceColorRecognizer::colorHist(cv::Mat &img, vector<pair<cv::Vec3b,cv::Vec3b>> &threshold_vec)
{
    //calculate color histogram of an image
    assert(img.channels() == 3);
    vector<int> hist(threshold_vec.size(),0);
    for(int i = 0; i < img.rows; i++)
    {
        
        for(int j = 0; j < img.cols; j++)
        {
            cv::Vec3b pixel = img.at<cv::Vec3b>(i, j); 
            
            for(int k = 0; k < threshold_vec.size(); k++)
            {
                if(isthisColor(pixel, threshold_vec[k]))
                {
                    hist[k]++;
                    break;
                }
            }
        }
    }
    return hist;
}
bool LicenceColorRecognizer::isthisColor(cv::Vec3b pixel, pair<cv::Vec3b, cv::Vec3b> threshold)
{
    //determine a pixel is in the color space or not
    if(pixel[0]>=threshold.first[0] && pixel[0]<=threshold.second[0]
        && pixel[1]>=threshold.first[1] && pixel[1]<=threshold.second[1] 
        && pixel[2]>=threshold.first[2] && pixel[2]<=threshold.second[2])
    {
        return true;
    }
    else
        return false;
}

pair<cv::Mat, cv::Mat> LicenceColorRecognizer::cluster(cv::Mat &img)
{ 
    //Kmeans:return amounts and centers of each kind color
    cv::cvtColor(img,img,cv::COLOR_BGR2HSV);  
    while(img.cols * img.rows > 1e5)
    {
        cv::resize(img, img, cv::Size(img.cols/2, img.rows/2));
    }
    cv::Mat samples(img.cols*img.rows, 1, CV_32FC3);

    cv::Mat labels(img.cols*img.rows, 1, CV_32SC1);
    uchar* p;   
    int i, j, k=0;   
    for(i=0; i < img.rows; i++)   
    {   
        p = img.ptr<uchar>(i);   
        for(j=0; j< img.cols; j++)   
        {   
            samples.at<cv::Vec3f>(k,0)[0] = float(p[j*3]);   
            samples.at<cv::Vec3f>(k,0)[1] = float(p[j*3+1]);   
            samples.at<cv::Vec3f>(k,0)[2] = float(p[j*3+2]);   
            k++;   
        }   
    }
    int clusterCount = 3;   
    cv::Mat centers(clusterCount, 1, samples.type());   
    cv::kmeans(samples, clusterCount, labels, cv::TermCriteria( cv::TermCriteria::EPS+cv::TermCriteria::MAX_ITER, 10, 0),   
        3, cv::KMEANS_PP_CENTERS, centers);
    cv::MatND hist;
    const int ch[]={0};
    const int histsize[]={clusterCount};
    float lrange[]={0.0,3.0};
    const float* ranges[]={lrange};
    labels.convertTo(labels,CV_8U);
    cv::calcHist(&labels,1,ch,cv::Mat(),hist,1,histsize,ranges,true,false);
    return pair<cv::Mat, cv::Mat>(hist,centers);  
}