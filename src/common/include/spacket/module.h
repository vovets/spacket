#pragma once

// Composable Protocol Modules

#include <spacket/executor.h>

#include <boost/intrusive/list.hpp>
#include <typedefs.h>


namespace bi = boost::intrusive;

struct Module;

struct StackOps {
    virtual Result<Module*> upper(Module& from) = 0;
    virtual Result<Module*> lower(Module& from) = 0;
};

using ModuleList = boost::intrusive::list<Module>;

class Module: public bi::list_base_hook<bi::link_mode<bi::normal_link>> {
    StackOps* ops_ = nullptr;
    alloc::Allocator& bufferAllocator_;
    alloc::Allocator& funcAllocator_;
    ExecutorI& executor_;

protected:
    StackOps& ops() {
        if (ops_ == nullptr) {
            FATAL_ERROR("Module::ops");
        }
        return *ops_;
    }

public:
    using ReadCompletionSig = Result<Void>(Result<Buffer>);
    using WriteCompletionSig = Result<Void>(Result<Void>);
    using ReadCompletionFunc = FunctionT<ReadCompletionSig>;
    using WriteCompletionFunc = FunctionT<WriteCompletionSig>;

    Module(alloc::Allocator& bufferAllocator, alloc::Allocator& funcAllocator, ExecutorI& executor)
        : bufferAllocator_(bufferAllocator)
        , funcAllocator_(funcAllocator)
        , executor_(executor)
    {}

protected:
    template <typename CompletionFunc, typename Operation>
    struct OperationT {
        using This = OperationT<CompletionFunc, Operation>;

        struct Deleter {
            void operator()(Operation* p) {
                p->~Operation();
                p->allocator.deallocate(p);
            };
        };
        
        using Ptr = std::unique_ptr<Operation, Deleter>;

        template <typename ...ArgTypes>
        static Result<Ptr> create(
            alloc::Allocator& allocator,
            CompletionFunc&& completion,
            ArgTypes... args
        ) {
            return
            alloc::allocate<Operation>(allocator) >=
            [&](Operation* p) {
                new (p) Operation(
                    allocator,
                    std::move(completion),
                    std::forward<ArgTypes>(args)...);
                return ok(Ptr(p));
            };
        }

        OperationT(alloc::Allocator& allocator, CompletionFunc&& completion)
            : allocator(allocator)
            , completion(std::move(completion))
        {}
        
        alloc::Allocator& allocator;
        CompletionFunc completion;
    };

    struct ReadOperation: OperationT<ReadCompletionFunc, ReadOperation> {
        using OperationT<ReadCompletionFunc, ReadOperation>::OperationT;
    };
    
    struct WriteOperation: OperationT<WriteCompletionFunc, WriteOperation> {
        Buffer buffer;

        WriteOperation(
            alloc::Allocator& allocator,
            WriteCompletionFunc&& completion,
            Buffer&& buffer
        )
            : OperationT<WriteCompletionFunc, WriteOperation>(
                allocator, std::move(completion)
            )
            , buffer(std::move(buffer))
        {}
    };

protected:
    ExecutorI& executor() { return executor_; }

    alloc::Allocator& bufferAllocator() { return bufferAllocator_; }

    alloc::Allocator& funcAllocator() { return funcAllocator_; }

    template <typename Sig, typename Closure>
    Result<FunctionT<Sig>> createFunction(Closure&& closure) {
        return
        FunctionT<Sig>::create(funcAllocator(), std::move(closure));
    }

    template <typename Closure>
    Result<Void> defer(Closure&& closure) {
        return
        createFunction<Result<Void>()>(std::move(closure)) >=
        [&](DeferredProc&& p) {
            return
            executor().defer(std::move(p));
        };
    }

    template <typename Operation, typename Result_>
    Result<Void> deferOperationCompletion(typename Operation::Ptr&& op_, Result_&& result) {
        return
        defer(
            [op=std::move(op_),r=std::move(result)]() mutable {
                return op->completion(std::move(r));
            }
        );
    }

    Result<Void> deferReadCompletion(ReadCompletionFunc&& completion, Buffer&& buffer) {
        return
        defer(
            [completion=std::move(completion),buffer=std::move(buffer)]() mutable {
                return completion(ok(std::move(buffer)));
            }
        );
    }

    Result<Void> deferWriteCompletion(WriteCompletionFunc&& completion) {
        return
        defer(
            [completion=std::move(completion)] () mutable {
                return completion(ok());
            }
        );
    }

public:
    void attach(StackOps* ops) {
        ops_ = ops;
    }
    
    virtual Result<Void> read(FunctionT<ReadCompletionSig>&& completion) {
        (void)completion;
        return fail(toError(ErrorCode::ModuleNotImplemented));
    }
    
    virtual Result<Void> write(Buffer&& b, FunctionT<WriteCompletionSig>&& completion) {
        (void)b;
        (void)completion;
        return fail(toError(ErrorCode::ModuleNotImplemented));
    }
};
