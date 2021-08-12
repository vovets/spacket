#pragma once

#include <spacket/result.h>
#include <spacket/allocator.h>

template <std::size_t N>
struct StaticDeferredProcT {
    using Retval = Result<Void>;
    using Storage = std::aligned_storage_t<sizeof(void*) * N, alignof(void*)>;

    struct FuncBase {
        virtual Retval operator()() = 0;
        virtual FuncBase* moveTo(Storage& storage) = 0;
        virtual void destroy() = 0;
        virtual ~FuncBase() {}
    };

    template <typename Closure>
    struct Func: FuncBase {
        Closure f;
        
        template <typename Closure_>
        explicit Func(Closure_&& f): f(std::forward<Closure_>(f)) {}

        Func(Func&& from): f(std::move(from.f)) {}

        void operator delete(void*, std::size_t) {}

        Retval operator()() override {
            return f();
        }

        FuncBase* moveTo(Storage& storage) override {
            return ::new (&storage) Func(std::move(*this));
        }

        void destroy() override {
            this->~Func();
        }
    };
        
    Storage storage;
    FuncBase* func;

    template <typename Closure>
    StaticDeferredProcT(Closure&& c)
        : func(::new (&storage) Func<Closure>(std::forward<Closure>(c)))
    {
        static_assert(sizeof(storage) >= sizeof(Func<Closure>));
    }

    ~StaticDeferredProcT() {
        if (func != nullptr) {
            func->destroy();
            func = nullptr;
        }
    }
    
    StaticDeferredProcT(const StaticDeferredProcT&) = delete;
    
    StaticDeferredProcT(StaticDeferredProcT&& from)
        : func(from.func ? from.func->moveTo(this->storage) : nullptr)
    {
        from.func = nullptr;
    }
    
    StaticDeferredProcT& operator=(StaticDeferredProcT&& from) {
        this->~StaticDeferredProcT();
        ::new (this) StaticDeferredProcT(std::move(from));
        return *this;
    }

    Result<Void> operator()() {
        return func->operator()();
    }
};

struct AllocatedDeferredProc {
    using Retval = Result<Void>;

    struct FuncBase {
        alloc::Allocator& allocator;

        FuncBase(alloc::Allocator& allocator): allocator(allocator) {}
        
        virtual Retval operator()() = 0;
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

        Retval operator()() override {
            return f();
        }

        void destroy() override {
            this->~Func();
        }
    };
        
    FuncBase* func;

    template <typename Closure>
    static Result<AllocatedDeferredProc> create(alloc::Allocator& allocator, Closure&& c) {
        using FuncType = Func<Closure>;
        return
        alloc::allocate<FuncType>(allocator) >=
        [&](FuncType* p) {
            return ok(AllocatedDeferredProc(p, allocator, std::move(c)));
        };
    }
    
    ~AllocatedDeferredProc() {
        if (func != nullptr) {
            alloc::Allocator& allocator = func->allocator;
            func->destroy();
            allocator.deallocate(func);
            func = nullptr;
        }
    }
    
    AllocatedDeferredProc(const AllocatedDeferredProc&) = delete;

    AllocatedDeferredProc(AllocatedDeferredProc&& from)
        : func(from.func)
    {
        from.func = nullptr;
    }
    
    AllocatedDeferredProc& operator=(AllocatedDeferredProc&& from) {
        this->~AllocatedDeferredProc();
        ::new (this) AllocatedDeferredProc(std::move(from));
        return *this;
    }

    Result<Void> operator()() {
        return func->operator()();
    }

private:
    template <typename Closure>
    AllocatedDeferredProc(void* storage, alloc::Allocator& allocator, Closure&& c)
        : func(::new (storage) Func<Closure>(allocator, std::forward<Closure>(c)))
    {
    }
};

using DeferredProc = AllocatedDeferredProc;
