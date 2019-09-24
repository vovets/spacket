#include <spacket/stm32/crc_device.h>
#include <spacket/debug_print.h>

#include <boost/endian/buffers.hpp>

namespace stm32 {

namespace {

IMPLEMENT_DPL_FUNCTION

}

std::uint32_t crc(CrcDevice& device, const std::uint8_t* data, std::size_t size) {
    device.reset();
    
    while (size >= 4) {
        device.add(reinterpret_cast<const boost::endian::big_uint32_buf_t*>(data)->value());
        data += 4;
        size -= 4;
    }

    std::uint32_t crc = device.last();
    std::uint32_t d = 0;

    // d = readBe(&data[i], size) >> (32 - size * 8)
    for (std::size_t i = 0; i < size; ++i) {
        d <<= 8;
        d = (d | data[i]);
    }

    std::uint32_t u2 = (crc >> (32 - size * 8)) ^ d;

    device.add(crc); // reset DR to 0
    device.add(u2);  // == [u2:00000000] mod g

    crc = device.last() ^ (crc << (size * 8));

    return crc;
}

} // stm32
