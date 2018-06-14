#include <spacket/stm32/crc_impl.h>

namespace stm32 {

namespace impl {

inline uint32_t readAsLE(const uint8_t* data, size_t size) {
    uint32_t result = 0;
    for (size_t i = 0; i < size; ++i) {
        result |= (static_cast<uint32_t>(data[i]) << (i * 8));
    }
    return result;
}

}

uint32_t CrcImpl::add(const uint8_t* data, size_t size) {
    while (size >= 4) {
        device->DR = *reinterpret_cast<const uint32_t*>(data);
        data += 4;
        size -= 4;
    }
    if (size) {
        uint32_t val = impl::readAsLE(data, size);
        uint8_t bits = size * 8;
        uint32_t crc = device->DR;
        val ^= crc ^ (crc >> (32 - bits));
        uint32_t finXor = crc << bits;
        device->DR = val;
        return device->DR ^ finXor;
    }
    return device->DR;
}

} // stm32
