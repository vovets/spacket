#pragma once

#include <spacket/result.h>
#include <spacket/format_utils.h>

#include <boost/preprocessor/cat.hpp>

#define returnOnFail(var, expr)                                         \
    auto BOOST_PP_CAT(var, __tmp) = expr;                               \
    if (isFail(BOOST_PP_CAT(var, __tmp))) {                             \
        using Type = SuccessT<decltype(expr)>;                          \
        return fail<Type>(getFailUnsafe(BOOST_PP_CAT(var, __tmp)));     \
    }                                                                   \
    auto var = std::move(getOkUnsafe(BOOST_PP_CAT(var, __tmp)))

#define returnOnFailT(var, type, expr)                                  \
    auto BOOST_PP_CAT(var, __tmp) = expr;                               \
    if (isFail(BOOST_PP_CAT(var, __tmp))) {                             \
        return fail<type>(getFailUnsafe(BOOST_PP_CAT(var, __tmp)));     \
    }                                                                   \
    auto var = std::move(getOkUnsafe(BOOST_PP_CAT(var, __tmp)))

#define returnOnFailE(expr)                                             \
    do {                                                                \
    const auto& BOOST_PP_CAT(var, __tmp) = expr;                        \
    if (isFail(BOOST_PP_CAT(var, __tmp))) {                             \
        using Type = SuccessT<decltype(expr)>;                          \
        return fail<Type>(getFailUnsafe(BOOST_PP_CAT(var, __tmp)));     \
    }                                                                   \
    } while(false)


inline
std::ostream& operator<<(std::ostream& os, const Hr<boost::blank>&) {
    return os;
}

template<typename Result>
typename std::enable_if_t<
std::is_same<typename Result::TypeId, result_impl::TypeId>::value,
std::ostream&>
operator<<(std::ostream& os, Hr<Result> hr_) {
    if (isOk(hr_.w)) {
        os << "ok[" << hr(getOkUnsafe(hr_.w)) << "]";
    } else {
        os << "fail[" << toString(getFailUnsafe(hr_.w)) << "]";
    }
    return os;
}
