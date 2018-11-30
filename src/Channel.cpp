#include <iostream>
#include <set>
#include <map>
#include <algorithm>

#include <opencv/cv.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>

#include "Channel.h"
#include "utils.h"

const std::string NAME = "name";
const std::string REGION = "region";
const std::string REGIONS = "regions";
const std::string CONFIG = "config";
const std::string TYPE = "type";
const std::string IN_ORIENTATION = "in_orientation";
Channel::Channel(LONG ch): Logging("Channel " + std::to_string(ch)) {
    channel = ch;
}

const int X = 0;
const int Y = 1;

std::vector<Region> getRegions(libconfig::Setting& setting) {
    std::vector<Region> regions;
    for (int i = 0; i < setting.getLength(); i++) {
        std::string name = setting[i][NAME];
        libconfig::Setting& region = setting[i][REGION];
        std::vector<cv::Point> points;
        for (int j = 0; j < region.getLength(); j++) {
            cv::Point p(region[j][X], region[j][Y]);
            points.push_back(p);
        }
        regions.push_back(Region(name, points));
    }
    return regions;
}

bool Channel::init(libconfig::Setting& config) {
    std::string n = config[NAME];
    name = n;

    std::string c = config[CONFIG];
    configName = c;

    regions = getRegions(config[REGIONS]);
    std::string t = config[TYPE];
    stationtype = t;
    std::string ori = config[IN_ORIENTATION];
    if(ori == "down"){
        in_orientation = 1;
    }else if(ori == "up"){
        in_orientation = -1;
    }
    ft2 -> loadFontData( "../../font/apple.ttf", 0 );
    logInfo("ch: " + std::to_string(channel) + " name: " + name + " config: " + configName);

    return true;
}

const std::string OUT_OF_ANY_REGION = "OUT_OF_ANY_REGION";

inline cv::Point center(cv::Rect& rect) {
    return cv::Point(rect.x + rect.width / 2, rect.y + rect.height / 2);
}

// find which region the target in, if not in any region, we make it in 'OUT_OF_ANY_REGION' region

RegionMap toRegion(std::vector<cv::Rect> targets, 
        std::vector<Region> regions, 
        LONG ch) {

    RegionMap results;
    for (cv::Rect& t: targets) {
        bool find = false;
        for (auto r: regions) {
            if (r.contains(center(t))) {
                results[r.name].push_back(t);
                find = true;
                break;
            }
        }
        if (!find) {
            results[OUT_OF_ANY_REGION].push_back(t);
        }
    }
    return results;
}

const std::string GATE = "gate";
const std::string DEFAULT = "station";

FrameInfo Channel::nextFrame (RegionMap& regionMap, FrameInfo& lastFrame, cv::Mat& src) {
    FrameInfo result;

    for (Region& r: regions) {
        result[r.name] = std::map<std::string, Target>();
    }

    // find in and stay
    for (auto kv: regionMap) {
        std::string regionName = kv.first;
        if (regionName == OUT_OF_ANY_REGION) {
            continue;
        }
        
        std::vector<cv::Rect>& rects = kv.second;
        for (cv::Rect& rect: rects) {
            Target candidate;
            candidate.setup(name, regionName, rect);
            auto targets = lastFrame[regionName];
            auto lookupResult = config -> lookup(src, candidate, targets);
            if (lookupResult.first) {
                Target& target = lookupResult.second;
                result[target.region][target.id] = target;
            }
        }
    }

    // mark and clean out
    for (auto rts: lastFrame) {
       auto ts = rts.second;
        for (auto tn: ts) {
            Target& t = tn.second;
            if (result[t.region].count(t.id) == 0 && !t.isOut()) {
                if (t.outCount > config -> outCount) {
                    if (stationtype == DEFAULT) {
                        t.markOut();
                        logDebug("markOut: " + t.toString());
                    } else if (stationtype == GATE) {
                        t.markStatus(in_orientation);
                        logDebug("markEnter: " + t.toString());
                    } else {
                        logDebug("error channels type");
                    }
                } else {
                    t.outOnce();
                    logDebug("outOnce: " + t.toString());
                }
                result[t.region][t.id] = t;
            }
        }
    }
    return result;
}

std::vector<std::string> STATUS = {"IN", "STAY", "OUT", "ENTER", "EXIT"};
cv::Scalar WHITE(255,255,255);
cv::Scalar RED(0, 0, 255);
cv::Scalar BLUE(255, 0, 0);
cv::Scalar GREEN(0, 255 ,0);

void Channel::render(cv::Mat& src, time_t& time) {
    for (auto r: regions) {
        // region box
        for (unsigned int i = 0; i < r.points.size(); i++) {
            cv::line (src, 
                    r.points[i],
                    r.points[(i + 1) % r.points.size()], 
                    BLUE,
                    2, 8);
        }
        // region name
        ft2 -> putText(src, 
                    r.name, 
                    r.points[0], 
                    30, RED, -1, cv::LINE_AA, false );
    }

    for (auto kv: lastFrame) {
        auto targets = kv.second;

        for (auto kv: targets) {
            auto t = kv.second;

            // target box
            cv::rectangle(src, t.rect, GREEN, 2, 8, 0);

            // target center
            cv::circle(src,
            t.center(),
            4,
            cv::Scalar(0, 255, 255),
            -1,
            8);

            ft2 -> putText(src,
                        STATUS[t.state],
                        t.center(),
                        30, RED, -1, cv::LINE_AA, false);

            cv::rectangle(src, t.prect, GREEN, 2, 8, 0);

            cv::Point license(t.pleft().x, t.pleft().y - 80);
            ft2 -> putText(src,
                        t.maxOccurrences(t.license), 
                        license,
                        30, RED, -1, cv::LINE_AA, false);
            cv::Point cmt(license.x, license.y + 30);
            ft2 -> putText(src,
                        t.maxOccurrences(t.color) + "," 
                            + t.maxOccurrences(t.make) + "," 
                            + t.maxOccurrences(t.type), 
                        cmt,
                        30, RED, -1, cv::LINE_AA, false);
            
            cv::Point id(cmt.x, cmt.y + 30);
            ft2 -> putText(src,
                        t.id, 
                        id,
                        30, RED, -1, cv::LINE_AA, false);
        }
    }
    writer -> writeDebugVideo(src);
}

void Channel::post(cv::Mat& src, time_t& time) {

   for (auto kv: lastFrame) {
        auto targets = kv.second;

        for (auto kv: targets) {
            logDebug(kv.second.toString());
            auto t = kv.second.toTargetInfo();
            t.time = time;
            db.writeTarget(t);
        }
   }
   db.writeRegionInfo(lastFrame, name);
   if (config -> videoDebug) {
       render(src, time);
   }
}

inline std::string rect2String(cv::Rect& r) {
    return "rect: " + std::to_string(r.x) + "," 
                            + std::to_string(r.y) + "," 
                            + std::to_string(r.width) + "," 
                            + std::to_string(r.height);
}

void Channel::doRun(cv::Mat& src) {
    auto start = std::chrono::steady_clock::now();

    cv::Rect origin(0, 0, src.cols - 1, src.rows - 1); 
    auto targetRects = CVModel::GetInstance().vehicle.predict(src, origin);
    for (auto r: targetRects) {
        logDebug(rect2String(r));
    }

    auto regionMap = toRegion(targetRects, regions, channel);
    for (auto kv: regionMap) {
        logDebug(kv.first);
        for (auto r: kv.second) {
            logDebug(rect2String(r));
        }
    }


    lastFrame = nextFrame(regionMap, lastFrame, src);

    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();

    avgtime += (time - avgtime) / (count + 1);

    count += 1;
    
    logDebug("time: " + std::to_string(time));
    logDebug("avg time: " + std::to_string(avgtime));    
}

void Channel::run() {
    while (running) {
        logDebug("queue size: " + std::to_string(q.size()));
        auto element = q.remove();
        cv::Mat m = element.first;
        time_t t = element.second;
        cv::Mat dist;
        cv::Mat& targetMat = convert(m, dist);
        doRun(targetMat);
        post(targetMat, t);
    }
}

void Channel::origRun() {
    while (running) {
        auto src = origQueue.remove();
        cv::Mat dist;
        cv::Mat& targetMat = convert(src, dist);
        writer -> writeOrigVideo(targetMat);
    }
}

bool Channel::stop() {
    running = false;
    return true;
}

bool Channel::start() {
    running = true;
    db.start();
    t = std::thread(&Channel::run, this);
    if (config -> videoOrig) {
        origThread = std::thread(&Channel::origRun, this);
    }
    return true;
}

void Channel::join() {
    t.join();
}

Channel::~Channel() {
    if (config) {
        delete config;
    }
    if (writer) {
        delete writer;
    }
}
