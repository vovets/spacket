#pragma once

#include <spacket/result.h>
#include <spacket/errors.h>
#include <spacket/impl/log_error_p.h>

namespace impl {

void logError(Error e);

} // impl

template <typename SuccessType>
Result<SuccessType> logErrorT(Error e) {
    impl::logError(e);
    return fail<SuccessType>(e);
}

inline
Result<Void> logError(Error e) {
    return logErrorT<Void>(e);
}
