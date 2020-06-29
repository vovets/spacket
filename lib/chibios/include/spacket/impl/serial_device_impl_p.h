#pragma once

#include <spacket/result.h>
#include <spacket/time_utils.h>
#include <spacket/mailbox.h>
#include <spacket/result_fatal.h>
#include <spacket/thread.h>
#include <spacket/util/thread_error_report.h>
#include <spacket/buffer_debug.h>
#include <spacket/allocator.h>

#include "hal.h"

#include <boost/intrusive_ptr.hpp>

#include <limits>

#if !defined(HAL_USE_UART) || HAL_USE_UART != TRUE
#error SerialDevice needs '#define HAL_USE_UART TRUE' in halconf.h \
    and '#define XXX_UART_USE_XXXX TRUE' in mcuconf.h
#endif

namespace serial_device_impl {

struct Debug {

#ifdef SERIAL_DEVICE_ENABLE_DEBUG_PRINT

    template <typename ...Args>
    static
    void dpl(Args&&... args) {
        debugPrintLine(std::forward<Args>(args)...);
    }

    static IMPLEMENT_DPB_FUNCTION

#else

    template <typename ...Args>
    static
    void dpl(Args&&...) {}

    static IMPLEMENT_DPB_FUNCTION_NOP

#endif

};

} // serial_device_impl

class SerialDeviceImpl: public serial_device_impl::Debug {
private:
    using Base = serial_device_impl::Debug;
    using This = SerialDeviceImpl;
    using Mailbox = MailboxT<Result<Buffer>>;
    using ThreadStorage = ThreadStorageT<280>;
    using Base::dpl;

public:
    template <typename SerialDevice>
    static Result<SerialDevice> open(alloc::Allocator& allocator, UARTDriver& driver, tprio_t threadPriority);

    void start(tprio_t threadPriority);
    void stop();

    SerialDeviceImpl(SerialDeviceImpl&& src) noexcept;
    SerialDeviceImpl& operator=(SerialDeviceImpl&& src) noexcept;

    ~SerialDeviceImpl();

    SerialDeviceImpl(const SerialDeviceImpl& src) = delete;
    SerialDeviceImpl& operator=(const SerialDeviceImpl& src) = delete;

    Result<Buffer> read(Timeout t);

    Result<Void> write(const uint8_t* buffer, size_t size, Timeout t);

    Result<Void> write(const uint8_t* buffer, size_t size) {
        return write(buffer, size, INFINITE_TIMEOUT);
    }

private:
    SerialDeviceImpl(UARTDriver& driver, Buffer&& dmaBuffer, tprio_t threadPriority);

    static void txend2_(UARTDriver*);
    static void rxIdle_(UARTDriver* driver);
    static void rxCompleted_(UARTDriver* driver);
    static void rxError_(UARTDriver *uartp, uartflags_t e);
    
    static void readThreadFunction_();
    static void handleReceivedData_(std::size_t received);

    void rxCompleted();
    void txCompleted();

private:
    static UARTConfig config;
    static SerialDeviceImpl* instance;
    static Mailbox readMailbox;
    static ThreadStorage readThreadStorage;
    static Thread readThread;
    static thread_reference_t readThreadRef;
    static thread_reference_t writeThreadRef;

private:
    UARTDriver* driver;
    Buffer readBuffer;
};


template <typename SerialDevice>
Result<SerialDevice> SerialDeviceImpl::open(alloc::Allocator& allocator, UARTDriver& driver, tprio_t threadPriority) {
    return
    Buffer::create(allocator) >=
    [&] (Buffer&& dmaBuffer) {
        return ok(SerialDevice(SerialDeviceImpl(driver, std::move(dmaBuffer), threadPriority)));
    } <=
    [] (Error error) { return fail<SerialDevice>(error); };
}

inline
SerialDeviceImpl::SerialDeviceImpl(UARTDriver& driver_, Buffer&& dmaBuffer, tprio_t threadPriority)
    : driver(&driver_)
    , readBuffer(std::move(dmaBuffer))
{
    instance = this;

    // without this callback driver won't enable TC interrupt
    // so uartSendFullTimeout will stuck forever
    config.txend2_cb = txend2_;
    config.rxend_cb = rxCompleted_;
    config.rxerr_cb = rxError_;
    config.timeout_cb = rxIdle_;
    config.speed = 921600;
    config.cr1 = USART_CR1_IDLEIE;
    
    start(threadPriority);
}

inline
SerialDeviceImpl::~SerialDeviceImpl() {
    if (driver != nullptr) {
        stop();
        instance = nullptr;
    }
}

inline
SerialDeviceImpl::SerialDeviceImpl(SerialDeviceImpl&& src) noexcept
    : driver(src.driver)
    , readBuffer(std::move(src.readBuffer))
{
    instance = this;
    src.driver = nullptr;
}
    
inline
SerialDeviceImpl& SerialDeviceImpl::operator=(SerialDeviceImpl&& src) noexcept {
    driver = src.driver;
    src.driver = nullptr;
    readBuffer = std::move(src.readBuffer);
    instance = this;
    return *this;
}

inline
void SerialDeviceImpl::start(tprio_t threadPriority) {
    uartStart(driver, &config);
    readThread = Thread::create(Thread::params(readThreadStorage, threadPriority), &This::readThreadFunction_);
}

inline
void SerialDeviceImpl::stop() {
    uartStopReceive(driver);
    readThread.requestStop();
    chThdResume(&readThreadRef, MSG_RESET);
    readThread.wait();
    uartStop(driver);
}

inline
Result<Buffer> SerialDeviceImpl::read(Timeout t) {
    return
    readMailbox.fetch(t) >=
    [] (Result<Buffer>&& result) {
        return std::move(result);
    } <=
    [] (Error e) {
        if (e == toError(ErrorCode::Timeout)) {
            return fail<Buffer>(toError(ErrorCode::ReadTimeout));
        }
        return fail<Buffer>(e);
    };
}

inline
Result<Void> SerialDeviceImpl::write(const uint8_t* buffer, size_t size, Timeout t) {
    osalSysLock();
    uartStartSendI(driver, size, buffer);
    auto msg = osalThreadSuspendTimeoutS(&writeThreadRef, toSystime(t));
    if (msg == MSG_TIMEOUT) {
        uartStopSendI(driver);
    }
    osalSysUnlock();
    dpl("sdi::write|msg=%d", msg);
    return msg == MSG_OK ? ok() : fail(toError(ErrorCode::WriteTimeout));
}

inline
void SerialDeviceImpl::txend2_(UARTDriver*) {
    osalSysLockFromISR();
    dpl("sdi::txend2_|instance=%x", instance);
    if (instance != nullptr) {
        instance->txCompleted();
    }
    osalSysUnlockFromISR();
}

inline
void SerialDeviceImpl::rxIdle_(UARTDriver* driver) {
    osalSysLockFromISR();
    dpl("sdi::rxIdle_|instance=%x", instance);
    if (instance != nullptr && driver->rxstate == UART_RX_ACTIVE) {
        // if we received exactly maxSize bytes then rxCompleted_ will fire
        // and then rxIdle_, so we do nothing in rxIdle_ in that case
        // because rxCompleted_ will handle everything
        //
        // note that we call rxCompleted here
        instance->rxCompleted();
    }
    osalSysUnlockFromISR();
}

inline
void SerialDeviceImpl::rxCompleted_(UARTDriver*) {
    osalSysLockFromISR();
    dpl("sdi::rxCompleted_|instance=%x", instance);
    if (instance != nullptr) {
        instance->rxCompleted();
    }
    osalSysUnlockFromISR();
}

inline
void SerialDeviceImpl::rxError_(UARTDriver*, uartflags_t) {
    dpl("sdi::rxError_");
}

inline
void SerialDeviceImpl::rxCompleted() {
    dpl("sdi::rxCompleted");
    auto notReceived = uartStopReceiveI(driver);
    osalThreadResumeI(&readThreadRef, notReceived);
}

inline
void SerialDeviceImpl::txCompleted() {
    auto notSent = uartStopSendI(driver);
    osalThreadResumeI(&writeThreadRef, notSent);
}

inline
void SerialDeviceImpl::handleReceivedData_(std::size_t notReceived) {
    dpl("sdi::handleReceivedData_|notReceived=%d", notReceived);
    alloc::Allocator& allocator = instance->readBuffer.allocator();
    Buffer::create(allocator) >=
    [&] (Buffer&& newBuffer) {
        instance->readBuffer.resize(instance->readBuffer.maxSize() - notReceived);
        auto message = ok(std::move(instance->readBuffer));
        return
            readMailbox.replace(message) >
            [&]() {
                instance->readBuffer = std::move(newBuffer);
                dpl("sdi::handleReceivedData_|replaced");
                return ok();
            };
    } <= threadErrorReport;
}

inline
void SerialDeviceImpl::readThreadFunction_() {
    Thread::setName("sd");
    while (!Thread::shouldStop()) {
        osalSysLock();
        uartStartReceiveI(instance->driver, instance->readBuffer.size(), instance->readBuffer.begin());
        auto result = osalThreadSuspendS(&readThreadRef);
        dpl("sdi::readThreadFunction_|result=%d", result);
        osalSysUnlock();
        if (result >= 0) {
            handleReceivedData_(result);
        }
    }
}
