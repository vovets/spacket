#pragma once

#include "errors.h"

#include <boost/variant.hpp>
#include <boost/mpl/at.hpp>

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
SuccessT<Result> getOk(Result& r, const char* file, int line) {
    auto p = boost::get<SuccessT<Result>>(&r);
    if (!p) {
        std::cerr                                              
            << "getOk called upon error result at: " 
            << file << ":" << line;                
        exit(1); }
    return std::move(*p);
}

template <typename Result>
ErrorT<Result> getFail(Result& r, const char* file, int line) {
    auto p = boost::get<ErrorT<Result>>(&r);
    if (!p) {
        std::cerr                                              
            << "getFail called upon success result at: " 
            << file << ":" << line;                
        exit(1); }
    return std::move(*p);
}

#define GETOK(X) getOk(X, __FILE__, __LINE__)
       
#define GETFAIL(X) getFail(X, __FILE__, __LINE__)
