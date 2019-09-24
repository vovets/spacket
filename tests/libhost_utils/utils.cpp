#include "utils.h"

#include <json.hpp>

#include <fstream>

namespace js = nlohmann;

PortConfig fromJson(const std::string& path) {
    std::ifstream i;
    i.open(path);
    if (!i.good()) {
        throw std::runtime_error("error opening " + path + " for reading");
    }
    js::json j;
    i >> j;
    return PortConfig{
        j.at("device"),
        fromInt(j.at("baud")),
        0};
}
