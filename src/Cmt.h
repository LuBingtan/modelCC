#ifndef _CMT_H_
#define _CMT_H_
#include <string>

class CMT {
public:
    std::string color;
    std::string make;
    std::string type;
    std::string toString() {
        std::string s;
        s.append(" color: " + color);
        s.append(" make: " + make);
        s.append(" type: " + type);
        return s;
    };
};

#endif