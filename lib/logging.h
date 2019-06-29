#pragma once

#include <iostream>

class Tracer {
public:
    ~Tracer() {
        std::cerr << std::endl;
    }
    
    template<typename T>
    std::ostream& operator<<(const T& t) {
        std::cerr << t;
        return std::cerr;
    }
};

#ifdef TRACE
#error macro 'TRACE' already defined
#endif

#ifdef SPACKET_ENABLE_TRACE
#define TRACE(E) do { Tracer() << E; } while(false)
#else
#define TRACE(E) do {} while(false)
#endif
