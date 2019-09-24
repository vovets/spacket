#include <spacket/impl/crc_p.h>
#include <spacket/stm32/crc_device.h>

namespace crc {

std::uint32_t crc(const std::uint8_t* buffer, std::size_t size) {
    static auto crcDevice = stm32::CrcDevice::open(CRC);
    return stm32::crc(crcDevice, buffer, size);
}

} // crc
