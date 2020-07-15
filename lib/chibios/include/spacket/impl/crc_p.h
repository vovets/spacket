#pragma once

#include <spacket/stm32/crc_device.h>

#include <cstdint>

namespace crc {

inline
std::uint32_t crc(const std::uint8_t* buffer, std::size_t size) {
    static auto crcDevice = stm32::CrcDevice::open(CRC);
    return stm32::crc(crcDevice, buffer, size);
}

} // crc
