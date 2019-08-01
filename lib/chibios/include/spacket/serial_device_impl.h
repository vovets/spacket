#pragma once

#include <spacket/result.h>
#include <spacket/time_utils.h>
#include <spacket/util/mailbox.h>
#include <spacket/result_fatal.h>
#include <spacket/util/static_thread.h>
#include <spacket/util/thread_error_report.h>

#include "hal.h"

#include <boost/intrusive_ptr.hpp>

#include <limits>
#if !defined(HAL_USE_UART) || HAL_USE_UART != TRUE
#error SerialDevice needs '#define HAL_USE_UART TRUE' in halconf.h \
    and '#define XXX_UART_USE_XXXX TRUE' in mcuconf.h
#endif

#if !defined(UART_USE_WAIT) || UART_USE_WAIT != TRUE
#error SerialDevice needs '#define UART_USE_WAIT' in halconf.h
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

#ifdef SERIAL_DEVICE_ENABLE_DEBUG_PRINT

IMPLEMENT_DPL_FUNCTION
IMPLEMENT_DPX_FUNCTIONS

#else

IMPLEMENT_DPL_FUNCTION_NOP
IMPLEMENT_DPX_FUNCTIONS_NOP

#endif

} // serial_device_impl

template <typename Buffer>
class SerialDeviceImpl {
private:
    using This = SerialDeviceImpl<Buffer>;
    using Mailbox = MailboxT<Result<Buffer>, 1>;
    using Thread = StaticThreadT<256>;

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

    Result<boost::blank> write(const uint8_t* buffer, size_t size, Timeout t);

    Result<boost::blank> write(const uint8_t* buffer, size_t size) {
        return write(buffer, size, INFINITE_TIMEOUT);
    }

private:
    SerialDeviceImpl(UARTDriver* driver, Buffer&& dmaBuffer, tprio_t threadPriority);
    static void txend2_(UARTDriver*);
    static void rxCompleted_(UARTDriver* driver);
    static void rxError_(UARTDriver *uartp, uartflags_t e);
    static void rxIdle_(UARTDriver* driver);
    static void readTimeout_(void*);
    static void readThreadFunction_(void*);
    static void handleReceivedData_(std::size_t received);
    static void handleTimeout_();

    void readTimeout();

private:
    static UARTConfig config;
    static SerialDeviceImpl* instance;
    static Mailbox readMailbox;
    static Thread readThread;
    static bool stopRequested;
    static virtual_timer_t readTimeoutTimer;

private:
    using Driver = boost::intrusive_ptr<UARTDriver>;
    Driver driver;
    Buffer readBuffer;
};


template <typename Buffer>
UARTConfig SerialDeviceImpl<Buffer>::config = {
    .txend1_cb = nullptr,
    // without this callback driver won't enable TC interrupt
    // so uartSendFullTimeout will stuck forever
    .txend2_cb = txend2_,
    .rxend_cb = rxCompleted_,
    .rxchar_cb = nullptr,
    .rxerr_cb = rxError_,
    .timeout_cb = rxIdle_,
    .speed = 921600,
//    .speed = 115200,
    .cr1 = USART_CR1_IDLEIE,
    .cr2 = 0,
    .cr3 = 0
};

template <typename Buffer>
SerialDeviceImpl<Buffer>* SerialDeviceImpl<Buffer>::instance = nullptr;

template <typename Buffer>
typename SerialDeviceImpl<Buffer>::Mailbox SerialDeviceImpl<Buffer>::readMailbox;

template <typename Buffer>
virtual_timer_t SerialDeviceImpl<Buffer>::readTimeoutTimer;

template <typename Buffer>
typename SerialDeviceImpl<Buffer>::Thread SerialDeviceImpl<Buffer>::readThread;

template <typename Buffer>
bool SerialDeviceImpl<Buffer>::stopRequested = false;

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
    , readBuffer(src.readBuffer)
{
    instance = this;
    src.driver.reset();
}
    
template <typename Buffer>
SerialDeviceImpl<Buffer>& SerialDeviceImpl<Buffer>::operator=(SerialDeviceImpl&& src) noexcept {
    driver = src.driver;
    readBuffer = src.readBuffer;
    instance = this;
    return *this;
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::start(tprio_t threadPriority) {
    uartStart(driver.get(), &config);
    stopRequested = false;
    auto thread = readThread.create(threadPriority, This::readThreadFunction_, nullptr);
    chRegSetThreadNameX(thread, "sd");
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::stop() {
    stopRequested = true;
    uartStopReceive(driver.get());
    chThdResume(&driver->threadrx, MSG_RESET);
    readThread.wait();
    uartStop(driver.get());
}

template <typename Buffer>
Result<Buffer> SerialDeviceImpl<Buffer>::read(Timeout t) {
    chVTSet(&readTimeoutTimer, t.count(), This::readTimeout_, nullptr);
    return
    readMailbox.fetch(infiniteTimeout()) >=
    [] (Result<Buffer>&& result) {
        return std::move(result);
    };
}

template <typename Buffer>
Result<boost::blank> SerialDeviceImpl<Buffer>::write(const uint8_t* buffer, size_t size, Timeout t) {
    auto msg = uartSendFullTimeout(driver.get(), &size, buffer, toSystime(t));
    return msg == MSG_OK ? ok(boost::blank()) : fail<boost::blank>(toError(ErrorCode::SerialDeviceWriteTimeout));
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::txend2_(UARTDriver*) {
    serial_device_impl::dpl("txend2_");
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::rxCompleted_(UARTDriver*) {
    serial_device_impl::dpl("rxCompleted_");
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::rxError_(UARTDriver*, uartflags_t) {
    serial_device_impl::dpl("rxError_");
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::rxIdle_(UARTDriver*) {
    serial_device_impl::dpl("rxIdle_");
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::readTimeout_(void*) {
    serial_device_impl::dpl("readTimeout_ instance=%x", instance);
    if (instance != nullptr) {
        instance->readTimeout();
    }
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::readTimeout() {
    osalSysLockFromISR();
    osalThreadResumeI(&driver->threadrx, MSG_TIMEOUT);
    osalSysUnlockFromISR();
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::handleReceivedData_(std::size_t received) {
    Buffer::create(Buffer::maxSize()) >=
    [&] (Buffer&& newBuffer) {
        return
        readMailbox.fetch(immediateTimeout()) >=
        [] (Result<Buffer>&& result) { auto tmp = std::move(result); return ok(boost::blank()); } <=
        [] (Error){ return ok(boost::blank()); } >
        [&] () {
            instance->readBuffer.resize(received);
            auto message = ok(std::move(instance->readBuffer));
            return
            readMailbox.post(message, immediateTimeout()) >
            [&]() {
                instance->readBuffer = std::move(newBuffer);
                return ok(boost::blank());
            };
        };
    } <= threadErrorReport;
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::handleTimeout_() {
    readMailbox.fetch(immediateTimeout()) >=
    [] (Result<Buffer>&& r) {
        auto tmp = std::move(r);
        serial_device_impl::dpl("handleTimeout_ readMailbox was full");
        return ok(boost::blank());
    } <=
    [] (Error e){
        serial_device_impl::dpl("handleTiemout_ %s", toString(e));
        (void)e;
        return ok(boost::blank());
    } >
    [&] () {
        auto message = fail<Buffer>(toError(ErrorCode::SerialDeviceReadTimeout));
        return
        readMailbox.post(message, immediateTimeout());
    } <=
    threadErrorReport;
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::readThreadFunction_(void*) {
    for (;;) {
        std::size_t received = Buffer::maxSize();
        auto result = uartReceiveTimeout(instance->driver.get(), &received, instance->readBuffer.begin(), TIME_INFINITE);
        serial_device_impl::dpl("readThreadFunction result=%d received=%d", result, received);
        chVTReset(&readTimeoutTimer);
        if (stopRequested) break;
        switch (result) {
            case MSG_OK:
                handleReceivedData_(received);
                break;
            case MSG_RESET:
                // TODO: log receive errror(s)
                break;
            case MSG_TIMEOUT:
                if (received > 0) {
                    handleReceivedData_(received);
                } else {
                    handleTimeout_();
                }
                break;
            default:
                FATAL_ERROR("uartReceiveTimeout returned unsupported result");
        }
    }
}

