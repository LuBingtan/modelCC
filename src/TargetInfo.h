#ifndef _TARGET_INFO_H_
#define _TARGET_INFO_H_

struct TargetInfo {
  std::string vid;
  time_t time;
  int state;
  int stayCount;
  std::string region;
  std::string channel; 
  std::string license;
  std::string type;
  std::string make;
  std::string color;

  std::string toString() {
    std::string s;
    s.append("vid: " + vid + ",");
    s.append("state: " + std::to_string(state) + ",");
    s.append("region: " + region + ",");
    s.append("channel: " + channel + ",");
    s.append("license: " + license + ",");
    s.append("type: " + type + ",");
    s.append("color: " + color + ".");
    return s;
  }
};

#endif