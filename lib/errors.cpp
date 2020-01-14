#include <spacket/errors.h>

#include <boost/preprocessor/stringize.hpp>

static const char* defaultErrorToStringHandler(Error) {
    return "Unknown error";
}

static ErrorToStringF errorToStringHandler = &defaultErrorToStringHandler;

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

ErrorToStringF setErrorToStringHandler(ErrorToStringF handler) {
    auto retval = errorToStringHandler;
    errorToStringHandler = handler;
    return retval;
}
