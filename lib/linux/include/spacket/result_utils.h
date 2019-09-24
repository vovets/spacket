#pragma once

#include <spacket/result_utils_base.h>

#include <exception>

template <typename Result>
SuccessT<Result> throwOnFail(Result r) {
    if (isFail(r)) {
        auto e = getFailUnsafe(r);
        throw std::runtime_error(toString(e));
    }
    return std::move(getOkUnsafe(std::move(r)));
}
