#include <spacket/fatal_error.h>

#include <printf.h>

#include <hal.h>


inline
const char* toString(Error e, char* buffer, std::size_t size) {
    int n = snprintf_(buffer, size, "%d.%d", e.source, e.code);
    if (n < 0) { FATAL_ERROR("toString"); }
    buffer[std::min(size - 1, static_cast<std::size_t>(n))] = 0;
    return buffer;
}
