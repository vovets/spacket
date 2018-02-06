#pragma once

#include <spacket/result.h>

template <typename R, typename F>
auto operator>=(R&& r, F f) {
    using FResult = std::result_of_t<F(SuccessT<R>)>;
    if (isOk(r)) {
        return f(getOkUnsafe(std::move(r)));
    }
    return fail<SuccessT<FResult>>(getFailUnsafe(r));
}

template <typename R, typename F>
auto operator>(R&& r, F f) {
    using FResult = std::result_of_t<F()>;
    if (isOk(r)) {
        return f();
    }
    return fail<SuccessT<FResult>>(getFailUnsafe(r));
}

template <typename R, typename F>
auto operator<=(R&& r, F f) -> std::result_of_t<F(ErrorT<R>)> {
    if (isFail(r)) {
        return f(getFailUnsafe(r));
    }
    return std::move(r);
}
