#pragma once

#include <spacket/config.h>

#include <string>

struct PortConfig {
    std::string device;
    Baud baud;
    size_t byteTimeout_us; // 0 means 2 * byte interval according to baud
};
