#pragma once

#include <cstdint>

namespace crc {

std::uint32_t crc(const std::uint8_t* buffer, std::size_t size);

} // crc
