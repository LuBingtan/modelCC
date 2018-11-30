#include <map>

#include "utils.h"
#include "Target.h"
#include "CVModel.h"

Target::Target() {}

cv::Point Target::center() const {
    return cv::Point(rect.x + rect.width / 2, rect.y + rect.height / 2);
}

cv::Point Target::pleft() {
    return cv::Point(rect.x, rect.y + rect.height);
}


inline std::string mkString(std::vector<std::string> vs) {
    std::string rs;
    for (auto s: vs) {
        rs.append(s);
        rs.append(",");
    }
    return rs;
}

std::string Target::toString() const {
    std::string s;
    s.append(" id: " + id);
    s.append(" state: " + std::to_string(state));
    s.append(" license: " + mkString(license));
    s.append(" channel: " + channel);
    s.append(" region: " + region);
    s.append(" type: " + mkString(type));
    s.append(" color: " + mkString(color));
    s.append(" make: " + mkString(make));
    s.append(" outcount: " + std::to_string(outCount));
    s.append(" stayCount: " + std::to_string(stayCount));
    return s;
}


void Target::stayOnce(cv::Rect& r, int stayCountInterval, cv::Mat& src) {
    stayCount += 1;
    outCount = 0;
    rect = r;
    state = STAY;
    addCoordinate(rect);
    if (stayCount % stayCountInterval == 0) {
        auto cmt = CVModel::GetInstance().predictColorMakeType(rect, src);
        auto ocr = CVModel::GetInstance().predictLicense(rect, src);
        addCmt(cmt);
        updateOcr(ocr);
    }
}


void Target::outOnce() {
    outCount++;
    stayCount++;
    state = STAY;
}

void Target::markOut() {
    state = OUT;
}

void Target::markIn() {
    addCoordinate(rect);
    state = IN;
}

bool Target::isOut(){
    return state == OUT || state == ENTER || state == EXIT;
}

void Target::markStatus(int in_orientation) {
    if (car_coordinate.size() > 1) {
        int diff = int(car_coordinate.back().y - car_coordinate.front().y);
        int ori = diff * in_orientation;
        printf("ori=: %d",ori);
        if (ori >= 0)
            state = ENTER;
        else if (ori < 0)
            state = EXIT;
    } else
        state = OUT;
    
}

std::string Target::maxOccurrences(std::vector<std::string>& vs) const {
    std::map<std::string, int> frequencyMap;
    int maxFrequency = 0;
    std::string mostFrequentElement;
    for (std::string x : vs) {
        if (!x.empty()) {
            int f = ++frequencyMap[x];
            if (f > maxFrequency) {
                maxFrequency = f;
                mostFrequentElement = x;
            }
        }
    }
    return mostFrequentElement;
}

const int MAX_REMEMBER = 50;

void Target::addCoordinate(cv::Rect& r) {
    car_coordinate.push_back(cv::Point(r.x + r.width/2, r.y + r.height/2));
    if (car_coordinate.size() > MAX_REMEMBER) {
        car_coordinate.erase(car_coordinate.begin());
    }
}

void Target::addCmt(CMT& c) {
    color.push_back(c.color);
    make.push_back(c.make);
    type.push_back(c.type);
    if (color.size() > MAX_REMEMBER) {
        color.erase(color.begin());
        make.erase(make.begin());
        type.erase(type.begin());
    }
}

void Target::updateOcr(std::pair<std::string, cv::Rect> ocr) {
        license.push_back(ocr.first);
        prect = ocr.second;
        if (license.size() > MAX_REMEMBER) {
            license.erase(license.begin());
        }        
}

const int id_size = 20;

void Target::setup(std::string ch, std::string re, cv::Rect& r) {
    channel = ch;
    region = re;
    rect = r;
    id = random_string(id_size);
    markIn();
}

TargetInfo Target::toTargetInfo() {
    TargetInfo info;
    info.vid = id;
    info.state = state;
    info.region = region;
    info.channel = channel;
    info.license = maxOccurrences(license);
    info.type = maxOccurrences(type);
    info.make = maxOccurrences(make);
    info.color = maxOccurrences(color);
    info.stayCount = stayCount;
    return info;
}

Target::~Target(){}