#pragma once

#include <spacket/result.h>


struct DeferredProc {
    using Retval = Result<Void>;
    using Storage = std::aligned_storage_t<sizeof(void*) * 5, alignof(void*)>;

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
    DeferredProc(Closure&& c)
        : func(::new (&storage) Func<Closure>(std::forward<Closure>(c)))
    {
        static_assert(sizeof(storage) >= sizeof(Func<Closure>));
    }

    ~DeferredProc() {
        if (func != nullptr) {
            func->destroy();
            func = nullptr;
        }
    }
    
    DeferredProc(const DeferredProc&) = delete;
    
    DeferredProc(DeferredProc&& from)
        : func(from.func ? from.func->moveTo(this->storage) : nullptr)
    {
        from.func = nullptr;
    }
    
    DeferredProc& operator=(DeferredProc&& from) {
        this->~DeferredProc();
        ::new (this) DeferredProc(std::move(from));
        return *this;
    }

    Result<Void> operator()() {
        return func->operator()();
    }
};
