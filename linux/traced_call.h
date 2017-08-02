#pragma once

#if defined(call) | defined(callv)
#error macro 'call or callv' already defined
#endif

#define callv(resultVar, function, fail, ...)                           \
    int resultVar = function(__VA_ARGS__);                              \
    do {                                                                \
        if (-1 == resultVar) {                                          \
            int err = errno;                                            \
            TRACE()                                                     \
                << #function << " failed with [" << err                 \
                << "]: " << strerror(err);                              \
            return fail();                                              \
        }                                                               \
    } while(false)

#define call(function, fail, ...)                                       \
    do {                                                                \
        int resultVar = function(__VA_ARGS__);                          \
        if (-1 == resultVar) {                                          \
            int err = errno;                                            \
            TRACE()                                                     \
                << #function << " failed with [" << err                 \
                << "]: " << strerror(err);                              \
            return fail();                                              \
        }                                                               \
    } while(false)
