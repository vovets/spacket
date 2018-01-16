#pragma once

#include <spacket/result.h>

template <typename R, typename F>
auto operator>>=(R r, F f) {
    using FResult = std::result_of_t<F(SuccessT<R>)>;
    if (isOk(r)) {
        return f(getOkUnsafe(r));
    }
    return fail<SuccessT<FResult>>(getFailUnsafe(r));
}
