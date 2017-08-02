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
    
#define TRACE() Tracer()
