#pragma once

#include <spacket/result.h>

template <typename R, typename F>
auto operator>>=(R r, F f) {
    using FResult = std::result_of_t<F(SuccessT<R>)>;
    if (isOk(r)) {
        return f(getOkUnsafe(std::move(r)));
    }
    return fail<SuccessT<FResult>>(getFailUnsafe(r));
}
