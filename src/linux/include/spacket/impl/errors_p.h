#include <spacket/errors.h>
#include <spacket/fatal_error.h>

#include <boost/preprocessor/stringize.hpp>

inline
const char* toString(ErrorCode e) {
    switch (e) {
#define X(ID, CODE, SEP) case ErrorCode::ID: return STR(ID);
#define STR(X) BOOST_PP_STRINGIZE(X)
        ERROR_LIST(ID, STR, ID)
#undef X
#undef STR
        default: return "unknown";
    }
}

inline
const char* toString(Error e, char* buffer, std::size_t size)
{
    int n = std::snprintf(buffer, size, "%d.%s", e.source, toString(ErrorCode(e.code)));
    if (n < 0) { FATAL_ERROR("toString"); }
    buffer[std::min(size - 1, static_cast<std::size_t>(n))] = 0;
    return buffer;
}
