#pragma once

#include <spacket/result.h>

#include <boost/preprocessor/cat.hpp>

#define returnOnFail(var, expr)                                         \
    auto BOOST_PP_CAT(var, __tmp) = expr;                               \
    if (isFail(BOOST_PP_CAT(var, __tmp))) {                             \
        using Type = SuccessT<decltype(expr)>;                          \
        return fail<Type>(getFailUnsafe(BOOST_PP_CAT(var, __tmp)));     \
    }                                                                   \
    auto var = getOkUnsafe(std::move(BOOST_PP_CAT(var, __tmp)))

#define returnOnFailT(var, type, expr)                                  \
    auto BOOST_PP_CAT(var, __tmp) = expr;                               \
    if (isFail(BOOST_PP_CAT(var, __tmp))) {                             \
        return fail<type>(getFailUnsafe(BOOST_PP_CAT(var, __tmp)));     \
    }                                                                   \
    auto var = getOkUnsafe(std::move(BOOST_PP_CAT(var, __tmp)))
