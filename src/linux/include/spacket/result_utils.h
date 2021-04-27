#pragma once

#include <spacket/result_utils_base.h>

#include <exception>
#include <sstream>

template <typename Result>
SuccessT<Result> throwOnFail(Result r) {
    if (isFail(r)) {
        auto e = getFailUnsafe(r);
        std::ostringstream ss;
        ss << e.source << "." << e.code << " " << toString(ErrorCode(e.code));
        throw std::runtime_error(ss.str());
    }
    return std::move(getOkUnsafe(std::move(r)));
}
