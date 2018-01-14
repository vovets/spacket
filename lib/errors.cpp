#include <spacket/errors.h>

#include <boost/preprocessor/stringize.hpp>

const char* toString(Error e) {
    switch (e) {
#define X(ID, CODE, SEP) case Error::ID: return CODE ":" STR(ID);
#define STR(X) BOOST_PP_STRINGIZE(X)
        ERROR_LIST(ID, STR, ID)
#undef X
#undef STR
        default: return "Unknown error";
    }
}
