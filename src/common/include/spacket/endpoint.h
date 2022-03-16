#pragma once

#include <spacket/module.h>


struct Endpoint: Module {
    ReadOperation::Ptr readOp;

    Result<Void> up(Buffer&& b) override {
        cpm::dpl("Endpoint::up|");
        readCompleted(std::move(b));
        return ok();
    }
    
    Result<Void> down(Buffer&& b) override {
        cpm::dpl("Endpoint::down|");
        return deferDown(std::move(b));
    }

    Result<Void> read(ReadCompletionFunc completion) override {
        cpm::dpl("Endpoint::read|");
        if (readOp) {
            return fail(toError(ErrorCode::ModuleOperationInProgress));
        }
        return
        createReadOperation(std::move(completion)) >=
        [&](ReadOperation::Ptr&& op) {
            return
            createFunction<ReadCompletionSig>(
                [&](Result<Buffer>&& r) {
                    return this->readCompleted(std::move(r));
                }
            ) >=
            [&](ReadCompletionFunc&& innerCompletion) {
                return
                ops().lower(*this) >=
                [&](Module* m) {
                    readOp = std::move(op);
                    return m->read(std::move(innerCompletion));
                };
            };
        };
    }

    Result<Void> readCompleted(Result<Buffer> result) {
        if (!readOp) { FATAL_ERROR("Endpoint::readCompleted"); return ok(); }
        return completeReadOperation(std::move(readOp), std::move(result));
    }
    
    Result<Void> write(FunctionT<WriteCompletionSig> completion) {
        cpm::dpl("Endpoint::write|");
        (void)completion;
        return ok();
    }
};
