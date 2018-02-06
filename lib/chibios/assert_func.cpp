#include <spacket/fatal_error.h>

extern "C" void __assert_func(const char* file , int line, const char* fname, const char* reason) {
    static_cast<void>(fname);
    fatalError(reason, file, line);
}
