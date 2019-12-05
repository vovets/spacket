#include <spacket/errors.h>

#include <boost/preprocessor/stringize.hpp>

static const char* defaultErrorToStringHandler(Error) {
    return "Unknown error";
}

static ErrorToStringF errorToStringHandler = &defaultErrorToStringHandler;

const char* toString(Error e) {
    switch (ErrorCode(e.code)) {
#ifdef SPACKET_NO_ERROR_STRINGS
#define X(ID, CODE, SEP) case ErrorCode::ID: return CODE;
#else
#define X(ID, CODE, SEP) case ErrorCode::ID: return CODE ":" STR(ID);
#endif
#define STR(X) BOOST_PP_STRINGIZE(X)
        ERROR_LIST(ID, STR, ID)
#undef X
#undef STR
        default: return errorToStringHandler(e);
    }
}

ErrorToStringF setErrorToStringHandler(ErrorToStringF handler) {
    auto retval = errorToStringHandler;
    errorToStringHandler = handler;
    return retval;
}
