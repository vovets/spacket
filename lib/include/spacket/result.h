#pragma once

#include <spacket/errors.h>
#include <spacket/fatal_error.h>

#include <memory>

namespace result_impl {

struct TypeId {};

} // result_impl


template <typename Success>
struct Result {
    using WhichType = bool;
    using TypeId = result_impl::TypeId;
    using ValueStorage = std::aligned_union_t<0, Error, Success>;
    using SuccessType = Success;
    using ErrorType = Error;

    WhichType isOk_;
    ValueStorage value_;

    Result(Error e)
        : isOk_(false) {
        new (&value_) Error{e};
    }

    Result(Success&& s)
        : isOk_(true) {
        new (&value_) Success{std::move(s)};
    }

    ~Result() {
        if (isOk_) {
            getOk().~Success();
        }
    }

    bool isOk() const { return isOk_; }

    Success& getOk() {
        return *reinterpret_cast<Success*>(&value_);
    }

    const Success& getOk() const {
        return *reinterpret_cast<const Success*>(&value_);
    }

    Error getFailure() const {
        return *reinterpret_cast<const Error*>(&value_);
    }

    Result(const Result&) = delete;

    Result(Result&& source) {
        constructFrom(source);
    }

    void swap(Result& other) {
        Result tmp(std::move(*this));
        constructFrom(other);
        other.constructFrom(tmp);
    }

    Result& operator=(Result&& other) {
        Result(std::move(other)).swap(*this);
        return *this;
    }

private:
    void constructFrom(Result& other) {
        isOk_ = other.isOk_;
        if (isOk_) {
            new (&value_) Success(std::move(other.getOk()));
        } else {
            new (&value_) Error(other.getFailure());
        }
    }
};

//template <typename Success>
//bool operator==(const Result<Success>& lhs, const Result<Success>& rhs) {
//    return lhs.value == rhs.value;
//}

template <typename Success>
Result<Success> ok(Success value) { return Result<Success>{std::move(value)}; }

template <typename Success>
Result<Success> fail(Error error) { return Result<Success>{std::move(error)}; }

template <typename Result>
bool isOk(const Result& r) { return r.isOk(); }

template <typename Result>
bool isFail(const Result& r) { return !r.isOk(); }

template <typename Result>
using SuccessT = typename Result::SuccessType;

template <typename Result>
using ErrorT = typename Result::ErrorType;

template <typename Result>
SuccessT<Result> getOkUnsafe(Result&& r) {
    return std::move(r.getOk());
}

template <typename Result>
const SuccessT<Result>& getOkUnsafe(const Result& r) {
    return r.getOk();
}

template <typename Result>
SuccessT<Result> getOkLoc(Result&& r, const char* file, int line) {
    if (!r.isOk()) {
        fatalError("getOk called upon error result", file, line);
    }
    return std::move(r.getOk());
}

template <typename Result>
const SuccessT<Result>& getOkLoc(const Result& r, const char* file, int line) {
    if (!r.isOk()) {
        fatalError("getOk called upon error result", file, line);
    }
    return r.getOk();
}

// template <typename Result>
// ErrorT<Result> getFailUnsafe(Result&& r) {
//     return std::move(*boost::get<ErrorT<Result>>(&r.value));
// }

// ErrorT<Result> is supposedly cheaper to copy than to move
template <typename Result>
ErrorT<Result> getFailUnsafe(const Result& r) {
    return r.getFailure();
}

template <typename Result>
ErrorT<Result> getFailLoc(const Result& r, const char* file, int line) {
    if (r.isOk()) {
        fatalError("getFail called upon success result", file, line);
    }
    return r.getFailure();
}
