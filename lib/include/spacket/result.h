#pragma once

#include "errors.h"

#include <boost/variant.hpp>
#include <boost/mpl/at.hpp>
#include <boost/preprocessor/cat.hpp>

#include <iostream>

template <typename Success>
using Result = boost::variant<Error, Success>;

template <typename Success>
Result<Success> ok(Success value) { return Result<Success>(std::move(value)); }

template <typename Success>
Result<Success> fail(Error error) { return Result<Success>(std::move(error)); }

template <typename Result>
bool isOk(const Result& r) { return !boost::get<Error>(&r); }

template <typename Result>
bool isFail(const Result& r) { return !isOk(r); }

template <typename Result>
using SuccessT = typename boost::mpl::at<typename Result::types, boost::mpl::int_<1>>::type;

template <typename Result>
using ErrorT = typename boost::mpl::at<typename Result::types, boost::mpl::int_<0>>::type;

template <typename Result>
SuccessT<Result> getOkUnsafe(Result& r) {
    return std::move(boost::get<SuccessT<Result>>(r));
}

template <typename Result>
SuccessT<Result> getOkLoc(Result& r, const char* file, int line) {
    auto p = boost::get<SuccessT<Result>>(&r);
    if (!p) {
        std::cerr                                              
            << "getOk called upon error result at: " 
            << file << ":" << line;                
        exit(1); }
    return std::move(*p);
}

template <typename Result>
ErrorT<Result> getFailUnsafe(Result& r) {
    return std::move(boost::get<ErrorT<Result>>(r));
}

template <typename Result>
ErrorT<Result> getFailLoc(Result& r, const char* file, int line) {
    auto p = boost::get<ErrorT<Result>>(&r);
    if (!p) {
        std::cerr                                              
            << "getFail called upon success result at: " 
            << file << ":" << line;                
        exit(1); }
    return std::move(*p);
}

template <typename Success>
Result<Success> idFail(Error e) { return fail<Success>(e); }


#define getOk(X) getOkLoc(X, __FILE__, __LINE__)
       
#define getFail(X) getFailLoc(X, __FILE__, __LINE__)

#define rcallv(var, fail, expr)                     \
    auto var##_result = expr;                       \
    if (isFail(var##_result)) {                     \
        return fail(getFailUnsafe(var##_result));   \
    }                                               \
    auto var = getOkUnsafe(var##_result)

#define rcall(fail, expr)                                           \
    auto BOOST_PP_CAT(result_,__LINE__) = expr;                     \
    if (isFail(BOOST_PP_CAT(result_,__LINE__))) {                   \
        return fail(getFailUnsafe(BOOST_PP_CAT(result_,__LINE__))); \
    }

#define nrcallv(var, fail, expr)           \
    auto var##_result = expr;      \
    if (isFail(var##_result)) {                     \
        fail(getFailUnsafe(var##_result));          \
    }                                               \
    auto var = getOkUnsafe(var##_result)

#define nrcall(fail, expr)                                  \
    auto BOOST_PP_CAT(result_,__LINE__) = expr;    \
    if (isFail(BOOST_PP_CAT(result_,__LINE__))) {                   \
        fail(getFailUnsafe(BOOST_PP_CAT(result_,__LINE__))); \
    }
