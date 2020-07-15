#include <spacket/fatal_error.h>

#include "osal.h"
#include "hal.h"
// #include "chprintf.h"

// #include <array>

inline
void fatalError(const char* reason, const char* file, int line) {
    (void)file;
    (void)line;
    osalSysHalt(reason);
}

// const char* toString(Error e) {
//     static std::array<char, 12> buffer;
//     chsnprintf(buffer.data(), buffer.size(), "%d:%d", e.source, e.code);
//     return buffer.data();
// }
