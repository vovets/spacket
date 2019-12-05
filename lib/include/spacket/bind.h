#pragma once

#include <spacket/result.h>
#include <spacket/thread.h>

template <typename SuccessType, typename F>
auto operator>=(Result<SuccessType>&& r, F f) {
    using FResult = std::result_of_t<F(SuccessType)>;
    SPACKET_THREAD_CHECK_STACK();
    if (isOk(r)) {
        SPACKET_THREAD_CHECK_STACK();
        return f(std::move(getOkUnsafe(std::move(r))));
    }
    SPACKET_THREAD_CHECK_STACK();
    return fail<SuccessT<FResult>>(getFailUnsafe(r));
}

template <typename R, typename F>
auto operator>(R&& r, F f) {
    using FResult = std::result_of_t<F()>;
    SPACKET_THREAD_CHECK_STACK();
    if (isOk(r)) {
        SPACKET_THREAD_CHECK_STACK();
        return f();
    }
    SPACKET_THREAD_CHECK_STACK();
    return fail<SuccessT<FResult>>(getFailUnsafe(r));
}

template <typename R, typename F>
typename std::enable_if_t<
    std::is_same<R, Result<SuccessT<R>>>::value,
    R>
operator<=(R&& r, F f) {
    SPACKET_THREAD_CHECK_STACK();
    if (isFail(r)) {
        SPACKET_THREAD_CHECK_STACK();
        return f(getFailUnsafe(r));
    }
    SPACKET_THREAD_CHECK_STACK();
    return std::move(r);
}
