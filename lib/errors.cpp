#include <spacket/errors.h>

#include <boost/preprocessor/stringize.hpp>


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
