#include "utils.h"

#include <json.hpp>

#include <fstream>

namespace js = nlohmann;

PortConfig fromJson(const std::string& path) {
    std::ifstream i(path);
    js::json j;
    i >> j;
    return PortConfig{
        j.at("device"),
        fromInt(j.at("baud")),
        0};
}
