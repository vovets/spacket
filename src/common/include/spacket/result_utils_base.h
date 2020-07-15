#pragma once

#include <spacket/result.h>
#include <spacket/format_utils.h>

#include <boost/preprocessor/cat.hpp>

#define returnOnFail(var, expr)                                         \
    auto BOOST_PP_CAT(var, __tmp) = std::move(expr);                    \
    if (isFail(BOOST_PP_CAT(var, __tmp))) {                             \
        using Type = SuccessT<decltype(expr)>;                          \
        return fail<Type>(getFailUnsafe(BOOST_PP_CAT(var, __tmp)));     \
    }                                                                   \
    auto var = std::move(getOkUnsafe(std::move(BOOST_PP_CAT(var, __tmp))))

#define returnOnFailT(var, type, expr)                                  \
    auto BOOST_PP_CAT(var, __tmp) = expr;                               \
    if (isFail(BOOST_PP_CAT(var, __tmp))) {                             \
        return fail<type>(getFailUnsafe(BOOST_PP_CAT(var, __tmp)));     \
    }                                                                   \
    auto var = std::move(getOkUnsafe(std::move(BOOST_PP_CAT(var, __tmp))))

#define returnOnFailE(expr)                                             \
    do {                                                                \
    const auto& BOOST_PP_CAT(var, __tmp) = expr;                        \
    if (isFail(BOOST_PP_CAT(var, __tmp))) {                             \
        using Type = SuccessT<decltype(expr)>;                          \
        return fail<Type>(getFailUnsafe(BOOST_PP_CAT(var, __tmp)));     \
    }                                                                   \
    } while(false)


template <typename Success>
Result<Success> idFail(Error e) { return fail<Success>(e); }

template<typename Result>
typename std::enable_if_t<
std::is_same<typename Result::TypeId, result_impl::TypeId>::value,
std::ostream&>
operator<<(std::ostream& os, Hr<Result> hr_) {
    if (isOk(hr_.w)) {
        os << std::string("ok[") << hr(getOkUnsafe(hr_.w)) << "]";
    } else {
        os << std::string("fail[") << toString(getFailUnsafe(hr_.w)) << "]";
    }
    return os;
}

template<typename Result>
typename std::enable_if_t<
std::is_same<typename Result::TypeId, result_impl::TypeId>::value,
bool>
operator==(const Result& lhs, const Result& rhs) {
    if (lhs.isOk() != rhs.isOk()) { return false; }
    if (lhs.isOk()) {
        return lhs.getOk() == rhs.getOk();
    } else {
        return lhs.getFailure() == rhs.getFailure();
    }
}

template<typename Result>
typename std::enable_if_t<
std::is_same<typename Result::TypeId, result_impl::TypeId>::value,
bool>
operator!=(const Result& lhs, const Result& rhs) {
    return !operator==(lhs, rhs);
}

struct Void {};

inline
bool operator==(const Result<Void>& lhs, const Result<Void>& rhs) {
    if (lhs.isOk() != rhs.isOk()) { return false; }
    return true;
}

inline
Result<Void> ok() { return ok(Void()); }

inline
Result<Void> fail(Error e) { return fail<Void>(e); }

inline
std::ostream& operator<<(std::ostream& os, const Hr<Void>&) {
    return os;
}
