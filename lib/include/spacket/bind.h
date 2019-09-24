#pragma once

#include <spacket/result.h>

template <typename SuccessType, typename F>
auto operator>=(Result<SuccessType>&& r, F f) {
    using FResult = std::result_of_t<F(SuccessType)>;
    if (isOk(r)) {
        return f(std::move(getOkUnsafe(std::move(r))));
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
typename std::enable_if_t<
    std::is_same<R, Result<SuccessT<R>>>::value,
    R>
operator<=(R&& r, F f) {
    if (isFail(r)) {
        return f(getFailUnsafe(r));
    }
    return std::move(r);
}
