#pragma once

#include <spacket/result.h>
#include <spacket/allocator.h>

template <typename Sig>
struct FunctionT;

template <typename ResultType, typename ...ArgTypes>
struct FunctionT<ResultType(ArgTypes...)> {
    struct FuncBase {
        alloc::Allocator& allocator;

        FuncBase(alloc::Allocator& allocator): allocator(allocator) {}
        
        virtual ResultType operator()(ArgTypes... args) = 0;
        virtual void destroy() = 0;
        virtual ~FuncBase() {}
    };

    template <typename Closure>
    struct Func: FuncBase {
        Closure f;
        
        template <typename Closure_>
        explicit Func(alloc::Allocator& allocator, Closure_&& f)
            : FuncBase(allocator)
            , f(std::forward<Closure_>(f))
        {
        }

        Func(Func&& from): f(std::move(from.f)) {}

        void operator delete(void*, std::size_t) {}

        ResultType operator()(ArgTypes... args) override {
            return f(std::forward<ArgTypes>(args)...);
        }

        void destroy() override {
            this->~Func();
        }
    };
        
    FuncBase* func;

    template <typename Closure>
    static Result<FunctionT<ResultType(ArgTypes...)>> create(alloc::Allocator& allocator, Closure&& c) {
        using FuncType = Func<Closure>;
        return
        alloc::allocate<FuncType>(allocator) >=
        [&](FuncType* p) {
            return ok(FunctionT<ResultType(ArgTypes...)>(p, allocator, std::move(c)));
        };
    }

    FunctionT(): func(nullptr) {}
    
    ~FunctionT() {
        if (func != nullptr) {
            alloc::Allocator& allocator = func->allocator;
            func->destroy();
            allocator.deallocate(func);
            func = nullptr;
        }
    }
    
    FunctionT(const FunctionT&) = delete;

    FunctionT(FunctionT&& from)
        : func(from.func)
    {
        from.func = nullptr;
    }
    
    FunctionT& operator=(FunctionT&& from) {
        this->~FunctionT();
        ::new (this) FunctionT(std::move(from));
        return *this;
    }

    ResultType operator()(ArgTypes... args) {
        return func->operator()(std::forward<ArgTypes>(args)...);
    }

private:
    template <typename Closure>
    FunctionT(void* storage, alloc::Allocator& allocator, Closure&& c)
        : func(::new (storage) Func<Closure>(allocator, std::forward<Closure>(c)))
    {
    }
};

using DeferredProc = FunctionT<Result<Void>()>;
