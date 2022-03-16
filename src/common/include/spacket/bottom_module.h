#pragma once

#include <spacket/module.h>
#include <spacket/driver.h>


struct BottomModule: public Module {
    Driver2& driver;
    ReadOperation::Ptr readOp;
    WriteOperation::Ptr writeOp;

    BottomModule(
        alloc::Allocator& bufferAllocator,
        alloc::Allocator& funcAllocator,
        ExecutorI& executor,
        Driver2& driver
    )
    : Module(bufferAllocator, funcAllocator, executor)
    , driver(driver)
    {}

    Result<Void> deferPoll() {
        return defer([&]() { return this->poll(); });
    }

    Result<Void> poll() {
        Driver2::Events events = driver.poll();
        if (events.has(Driver2::Event::RX_NEEDS_SERVICE)) { driver.serviceRx() <= logError; }
        if (events.has(Driver2::Event::TX_NEEDS_SERVICE)) { driver.serviceTx() <= logError; }
        if (events.has(Driver2::Event::RX_READY)) { completeRead() <= logError; }
        if (events.has(Driver2::Event::TX_READY)) { completeWrite() <= logError; }
        return deferPoll();
    }

    Result<Void> completeRead() {
        if (!readOp) {
            cpm::dpl("BottomModule::completeRead|drop");
            driver.rx();
            return driver.serviceRx();
        }
        auto r = driver.rx();
        cpm::dpl("BottomModule::completeRead|completing");
        return
        deferOperationCompletion<ReadOperation>(std::move(readOp), std::move(r)) >
        [&]() { return driver.serviceRx(); };
    }

    Result<Void> completeWrite() {
        if (!writeOp) {
            return ok();
        }
        auto r = driver.tx(std::move(writeOp->buffer));
        cpm::dpl("BottomModule::completeWrite|completing");
        return
        deferOperationCompletion<WriteOperation>(std::move(writeOp), std::move(r));
    }

    Result<Void> read(ReadCompletionFunc&& completion) override {
        cpm::dpl("Bottom::read|");
        if (driver.poll().has(Driver2::Event::RX_READY)) {
            return
            driver.rx() >=
            [&](Buffer&& b) {
                return
                deferReadCompletion(std::move(completion), std::move(b));
            };
        }
        return
        ReadOperation::create(bufferAllocator(), std::move(completion)) >=
        [&](ReadOperation::Ptr&& op) {
            readOp = std::move(op);
            return ok();
        };
    }

    Result<Void> write(Buffer&& buffer, WriteCompletionFunc&& completion) override {
        cpm::dpb("BottomModule::write|", &buffer);
        if (driver.poll().has(Driver2::Event::TX_READY)) {
            return
            driver.tx(std::move(buffer)) >
            [&]() {
                cpm::dpl("BottomModule::write|completing");
                return deferWriteCompletion(std::move(completion));
            } >
            [&]() { return driver.serviceTx(); };
        }
        return
        WriteOperation::create(bufferAllocator(), std::move(completion), std::move(buffer)) >=
        [&] (WriteOperation::Ptr&& op) {
            writeOp = std::move(op);
            return ok();
        };
    }
};
