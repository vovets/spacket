#include <spacket/config.h>

Baud fromInt(std::size_t b) {
    switch (b) {
#define BAUD(X) case X: return Baud::B_##X
#define X(BAUD, SEP) BAUD SEP
        BAUD_LIST(BAUD, SEP_SEMICOLON);
#undef X
#undef BAUD        
        default: return Baud::NonStandard;
    }
}
