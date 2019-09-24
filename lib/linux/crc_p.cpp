#include <spacket/impl/crc_p.h>
#include <crc.cpp.h>


namespace crc {

std::uint32_t crc(const std::uint8_t* buffer, std::size_t size) {
    std::uint32_t crc = crc_init();
    crc = crc_update(crc, buffer, size);
    return crc_finalize(crc);
}

} // crc
