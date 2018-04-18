#pragma once

#include <spacket/errors.h>
#include <spacket/fatal_error.h>

#include <boost/variant.hpp>
#include <boost/mpl/at.hpp>
#include <boost/preprocessor/cat.hpp>

template <typename Success>
struct Result {
    using ValueType = boost::variant<Error, Success>;
    ValueType value;
};

template <typename Success>
bool operator==(const Result<Success>& lhs, const Result<Success>& rhs) {
    return lhs.value == rhs.value;
}

template <typename Success>
Result<Success> ok(Success value) { return Result<Success>{std::move(value)}; }

template <typename Success>
Result<Success> fail(Error error) { return Result<Success>{std::move(error)}; }

template <typename Result>
bool isOk(const Result& r) { return !boost::get<Error>(&r.value); }

template <typename Result>
bool isFail(const Result& r) { return !isOk(r); }

template <typename Result>
using SuccessT = typename boost::mpl::at<typename Result::ValueType::types, boost::mpl::int_<1>>::type;

template <typename Result>
using ErrorT = typename boost::mpl::at<typename Result::ValueType::types, boost::mpl::int_<0>>::type;

template <typename Result>
SuccessT<Result>& getOkUnsafe(Result& r) {
    return *boost::get<SuccessT<Result>>(&r.value);
}

template <typename Result>
const SuccessT<Result>& getOkUnsafe(const Result& r) {
    return *boost::get<SuccessT<Result>>(&r.value);
}

template <typename Result>
SuccessT<Result> getOkLoc(Result&& r, const char* file, int line) {
    auto p = boost::get<SuccessT<Result>>(&r.value);
    if (!p) {
        fatalError("getOk called upon error result", file, line);
    }
    return *p;
}

template <typename Result>
const SuccessT<Result>& getOkLoc(const Result& r, const char* file, int line) {
    auto p = boost::get<SuccessT<Result>>(&r.value);
    if (!p) {
        fatalError("getOk called upon error result", file, line);
    }
    return *p;
}

// template <typename Result>
// ErrorT<Result> getFailUnsafe(Result&& r) {
//     return std::move(*boost::get<ErrorT<Result>>(&r.value));
// }

// ErrorT<Result> is supposedly cheaper to copy than to move
template <typename Result>
ErrorT<Result> getFailUnsafe(const Result& r) {
    return *boost::get<ErrorT<Result>>(&r.value);
}

template <typename Result>
ErrorT<Result> getFailLoc(const Result& r, const char* file, int line) {
    auto p = boost::get<ErrorT<Result>>(&r.value);
    if (!p) {
        fatalError("getFail called upon success result", file, line);
    }
    return *p;
}

template <typename Success>
Result<Success> idFail(Error e) { return fail<Success>(e); }

#define getOk(X) getOkLoc(X, __FILE__, __LINE__)
       
#define getFail(X) getFailLoc(X, __FILE__, __LINE__)
