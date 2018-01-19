#pragma once

#include <spacket/result_utils_base.h>

#include <exception>

template <typename Result>
SuccessT<Result> throwOnFail(Result r) {
    if (isFail(r)) {
        throw std::runtime_error(toString(getFailUnsafe(r)));
    }
    return getOkUnsafe(std::move(r));
}
