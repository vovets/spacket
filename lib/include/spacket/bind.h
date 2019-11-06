#pragma once

#include <spacket/result.h>
#include <spacket/thread.h>

template <typename SuccessType, typename F>
auto operator>=(Result<SuccessType>&& r, F f) {
    using FResult = std::result_of_t<F(SuccessType)>;
    Thread::checkStack();
    if (isOk(r)) {
        Thread::checkStack();
        return f(std::move(getOkUnsafe(std::move(r))));
    }
    Thread::checkStack();
    return fail<SuccessT<FResult>>(getFailUnsafe(r));
}

template <typename R, typename F>
auto operator>(R&& r, F f) {
    using FResult = std::result_of_t<F()>;
    Thread::checkStack();
    if (isOk(r)) {
        Thread::checkStack();
        return f();
    }
    Thread::checkStack();
    return fail<SuccessT<FResult>>(getFailUnsafe(r));
}

template <typename R, typename F>
typename std::enable_if_t<
    std::is_same<R, Result<SuccessT<R>>>::value,
    R>
operator<=(R&& r, F f) {
    Thread::checkStack();
    if (isFail(r)) {
        Thread::checkStack();
        return f(getFailUnsafe(r));
    }
    Thread::checkStack();
    return std::move(r);
}
