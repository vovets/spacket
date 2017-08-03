#pragma once

#if defined(call) | defined(callv)
#error macro 'call or callv' already defined
#endif

#define callv(resultVar, fail, function, ...)                           \
    int resultVar = function(__VA_ARGS__);                              \
    do {                                                                \
        while (-1 == resultVar) {                                       \
            int err = errno;                                            \
            if (err == EINTR) {                                         \
                resultVar = function(__VA_ARGS__);                      \
                continue;                                               \
            }                                                           \
            TRACE()                                                     \
                << #function << " failed with [" << err                 \
                << "]: " << strerror(err);                              \
            return fail();                                              \
        }                                                               \
    } while(false)

#define call(fail, function, ...)                                       \
    do {                                                                \
        int resultVar = function(__VA_ARGS__);                          \
        while (-1 == resultVar) {                                       \
            int err = errno;                                            \
            if (err == EINTR) {                                         \
                resultVar = function(__VA_ARGS__);                      \
                continue;                                               \
            }                                                           \
            TRACE()                                                     \
                << #function << " failed with [" << err                 \
                << "]: " << strerror(err);                              \
            return fail();                                              \
        }                                                               \
    } while(false)
