#include <spacket/result_fatal.h>

#include <array>

const char* toStringFatal(Error e) {
    static std::array<char, 12> buffer;
    return toString(e, buffer.data(), buffer.size());
}
