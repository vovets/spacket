#include <spacket/fatal_error.h>

#include <iostream>

void fatalError(const char* reason, const char* file, int line) {
    std::cerr << "FATAL at " << file << ":" << line << ": " << reason;
    exit(1);
}
