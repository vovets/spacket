#pragma once

#include <spacket/result.h>
#include <spacket/time_utils.h>
#include <spacket/mailbox.h>
#include <spacket/result_fatal.h>
#include <spacket/thread.h>
#include <spacket/util/thread_error_report.h>
#include <spacket/buffer_debug.h>

#include "hal.h"

#include <boost/intrusive_ptr.hpp>

#include <limits>

#if !defined(HAL_USE_UART) || HAL_USE_UART != TRUE
#error SerialDevice needs '#define HAL_USE_UART TRUE' in halconf.h \
    and '#define XXX_UART_USE_XXXX TRUE' in mcuconf.h
#endif

inline
void intrusive_ptr_add_ref(UARTDriver* d) {
    if (d->refCnt < std::numeric_limits<decltype(d->refCnt)>::max()) {
        ++d->refCnt;
    }
}

inline
void intrusive_ptr_release(UARTDriver* d) {
    --d->refCnt;
    if (d->refCnt == 0) {
        uartStop(d);
    }
}

namespace serial_device_impl {

template <typename Buffer>
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

template <typename Buffer>
class SerialDeviceImpl: public serial_device_impl::Debug<Buffer> {
private:
    using Base = serial_device_impl::Debug<Buffer>;
    using This = SerialDeviceImpl<Buffer>;
    using Mailbox = MailboxT<Result<Buffer>>;
    using ThreadStorage = ThreadStorageT<280>;
    using Base::dpl;

public:
    template <typename SerialDevice>
    static Result<SerialDevice> open(UARTDriver* driver, tprio_t threadPriority);

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
    SerialDeviceImpl(UARTDriver* driver, Buffer&& dmaBuffer, tprio_t threadPriority);

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
    using Driver = boost::intrusive_ptr<UARTDriver>;
    Driver driver;
    Buffer readBuffer;
};


template <typename Buffer>
UARTConfig SerialDeviceImpl<Buffer>::config; // zero-initialized

template <typename Buffer>
SerialDeviceImpl<Buffer>* SerialDeviceImpl<Buffer>::instance;

template <typename Buffer>
typename SerialDeviceImpl<Buffer>::Mailbox SerialDeviceImpl<Buffer>::readMailbox;

template <typename Buffer>
thread_reference_t SerialDeviceImpl<Buffer>::readThreadRef;

template <typename Buffer>
thread_reference_t SerialDeviceImpl<Buffer>::writeThreadRef;

template <typename Buffer>
Thread SerialDeviceImpl<Buffer>::readThread;

template <typename Buffer>
typename SerialDeviceImpl<Buffer>::ThreadStorage SerialDeviceImpl<Buffer>::readThreadStorage;

template <typename Buffer>
template <typename SerialDevice>
Result<SerialDevice> SerialDeviceImpl<Buffer>::open(UARTDriver* driver, tprio_t threadPriority) {
    if (driver->refCnt > 0) {
        return fail<SerialDevice>(toError(ErrorCode::SerialDeviceAlreadyOpened));
    }
    return
    Buffer::create(Buffer::maxSize()) >=
    [&] (Buffer&& dmaBuffer) {
        return ok(SerialDevice(SerialDeviceImpl(driver, std::move(dmaBuffer), threadPriority)));
    } <=
    [] (Error error) { return fail<SerialDevice>(error); };
}

template <typename Buffer>
SerialDeviceImpl<Buffer>::SerialDeviceImpl(UARTDriver* driver_, Buffer&& dmaBuffer, tprio_t threadPriority)
    : driver(driver_)
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

template <typename Buffer>
SerialDeviceImpl<Buffer>::~SerialDeviceImpl() {
    if (driver) {
        stop();
        instance = nullptr;
    }
}

template <typename Buffer>
SerialDeviceImpl<Buffer>::SerialDeviceImpl(SerialDeviceImpl&& src) noexcept
    : driver(src.driver)
    , readBuffer(std::move(src.readBuffer))
{
    instance = this;
    src.driver.reset();
}
    
template <typename Buffer>
SerialDeviceImpl<Buffer>& SerialDeviceImpl<Buffer>::operator=(SerialDeviceImpl&& src) noexcept {
    driver = src.driver;
    readBuffer = std::move(src.readBuffer);
    instance = this;
    return *this;
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::start(tprio_t threadPriority) {
    uartStart(driver.get(), &config);
    readThread = Thread::create(Thread::params(readThreadStorage, threadPriority), &This::readThreadFunction_);
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::stop() {
    uartStopReceive(driver.get());
    readThread.requestStop();
    chThdResume(&readThreadRef, MSG_RESET);
    readThread.wait();
    uartStop(driver.get());
}

template <typename Buffer>
Result<Buffer> SerialDeviceImpl<Buffer>::read(Timeout t) {
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

template <typename Buffer>
Result<Void> SerialDeviceImpl<Buffer>::write(const uint8_t* buffer, size_t size, Timeout t) {
    osalSysLock();
    uartStartSendI(driver.get(), size, buffer);
    auto msg = osalThreadSuspendTimeoutS(&writeThreadRef, toSystime(t));
    if (msg == MSG_TIMEOUT) {
        uartStopSendI(driver.get());
    }
    osalSysUnlock();
    dpl("sdi::write|msg=%d", msg);
    return msg == MSG_OK ? ok() : fail(toError(ErrorCode::WriteTimeout));
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::txend2_(UARTDriver*) {
    osalSysLockFromISR();
    dpl("sdi::txend2_|instance=%x", instance);
    if (instance != nullptr) {
        instance->txCompleted();
    }
    osalSysUnlockFromISR();
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::rxIdle_(UARTDriver* driver) {
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

template <typename Buffer>
void SerialDeviceImpl<Buffer>::rxCompleted_(UARTDriver*) {
    osalSysLockFromISR();
    dpl("sdi::rxCompleted_|instance=%x", instance);
    if (instance != nullptr) {
        instance->rxCompleted();
    }
    osalSysUnlockFromISR();
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::rxError_(UARTDriver*, uartflags_t) {
    dpl("sdi::rxError_");
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::rxCompleted() {
    dpl("sdi::rxCompleted");
    auto notReceived = uartStopReceiveI(driver.get());
    osalThreadResumeI(&readThreadRef, notReceived);
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::txCompleted() {
    auto notSent = uartStopSendI(driver.get());
    osalThreadResumeI(&writeThreadRef, notSent);
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::handleReceivedData_(std::size_t notReceived) {
    dpl("sdi::handleReceivedData_|notReceived=%d", notReceived);
    Buffer::create(Buffer::maxSize()) >=
    [&] (Buffer&& newBuffer) {
        instance->readBuffer.resize(Buffer::maxSize() - notReceived);
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

template <typename Buffer>
void SerialDeviceImpl<Buffer>::readThreadFunction_() {
    Thread::setName("sd");
    while (!Thread::shouldStop()) {
        osalSysLock();
        uartStartReceiveI(instance->driver.get(), Buffer::maxSize(), instance->readBuffer.begin());
        auto result = osalThreadSuspendS(&readThreadRef);
        dpl("sdi::readThreadFunction_|result=%d", result);
        osalSysUnlock();
        if (result >= 0) {
            handleReceivedData_(result);
        }
    }
}
