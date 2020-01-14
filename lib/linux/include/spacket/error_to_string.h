#pragma once

#include <sstream>

inline
std::string toString(Error e) {
    std::ostringstream ss;
    ss << e.source << "." << e.code << " " << toString(ErrorCode(e.code));
    return ss.str();
}
