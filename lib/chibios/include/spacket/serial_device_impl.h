#pragma once

#include <spacket/result.h>
#include <spacket/time_utils.h>
#include <spacket/util/mailbox.h>
#include <include/spacket/result_fatal.h>

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

template <typename Buffer>
class SerialDeviceImpl {
public:
    using Mailbox = MailboxT<Buffer, 1>;

private:
    using This = SerialDeviceImpl<Buffer>;

public:
    template <typename SerialDevice>
    static Result<SerialDevice> open(UARTDriver* driver, Mailbox& mailbox) {
        if (driver->refCnt > 0) {
            return fail<SerialDevice>(toError(ErrorCode::DevAlreadyOpened));
        }
        return
        Buffer::create(Buffer::maxSize()) >=
        [&] (Buffer&& dmaBuffer) {
            auto result = SerialDeviceImpl(driver, mailbox, std::move(dmaBuffer));
            result.start();
            return ok(SerialDevice(std::move(result)));
        } <=
        [] (Error error) { return fail<SerialDevice>(error); };
    }

    void start();
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

    Result<boost::blank> flush() { return ok(boost::blank{}); }

private:
    SerialDeviceImpl(UARTDriver* driver, Mailbox& mailbox, Buffer&& dmaBuffer);
    static void txend2(UARTDriver*);
    static void rxCompleted_(UARTDriver* driver);
    static void rxError_(UARTDriver *uartp, uartflags_t e);
    static void rxIdle_(UARTDriver* driver);
    static void readTimeout_(void*);
    void rxCompleted(std::size_t received);
    void rxError(uartflags_t) {}
    void rxIdle();
    void readTimeout();

private:
    static UARTConfig config;
    static SerialDeviceImpl* instance;

private:
    using Driver = boost::intrusive_ptr<UARTDriver>;
    Driver driver;
    Mailbox* readMailbox;
    Buffer readBuffer;
    virtual_timer_t readTimeoutTimer;
};


template <typename Buffer>
UARTConfig SerialDeviceImpl<Buffer>::config = {
    .txend1_cb = nullptr,
    // without this callback driver won't enable TC interrupt
    // so uartSendFullTimeout will stuck forever
    .txend2_cb = txend2,
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
SerialDeviceImpl<Buffer>::SerialDeviceImpl(UARTDriver* driver_, Mailbox& mailbox, Buffer&& dmaBuffer)
    : driver(driver_)
    , readMailbox(&mailbox)
    , readBuffer(std::move(dmaBuffer))
{
    instance = this;
    uartStart(driver.get(), &config);
    uartStartReceive(driver.get(), Buffer::maxSize(), readBuffer.begin());
}

template <typename Buffer>
SerialDeviceImpl<Buffer>::~SerialDeviceImpl() {
    if (driver) {
        uartStopReceive(driver.get());
        uartStop(driver.get());
        instance = nullptr;
    }
    readMailbox = nullptr;
}

template <typename Buffer>
SerialDeviceImpl<Buffer>::SerialDeviceImpl(SerialDeviceImpl&& src) noexcept
    : driver(src.driver)
    , readMailbox(src.readMailbox)
    , readBuffer(src.readBuffer)
{
    instance = this;
    src.driver.reset();
}
    
template <typename Buffer>
SerialDeviceImpl<Buffer>& SerialDeviceImpl<Buffer>::operator=(SerialDeviceImpl&& src) noexcept {
    driver = std::move(src.driver);
    return *this;
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::start() {
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::stop() {
}

template <typename Buffer>
Result<Buffer> SerialDeviceImpl<Buffer>::read(Timeout t) {
    return readMailbox->fetch(t);
}

template <typename Buffer>
Result<boost::blank> SerialDeviceImpl<Buffer>::write(const uint8_t* buffer, size_t size, Timeout t) {
    auto msg = uartSendFullTimeout(driver.get(), &size, buffer, toSystime(t));
    return msg == MSG_OK ? ok(boost::blank()) : fail<boost::blank>(toError(ErrorCode::DevWriteTimeout));
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::txend2(UARTDriver*) {
//    debugPrintLine("txend2");
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::rxCompleted_(UARTDriver*) {
//    debugPrintLine("rxCompleted_");
    if (instance) {
        instance->rxCompleted(Buffer::maxSize());
    }
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::rxError_(UARTDriver*, uartflags_t e) {
//    debugPrintLine("rxError_");
    if (instance) {
        instance->rxError(e);
    }
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::rxIdle_(UARTDriver*) {
//    debugPrintLine("rxIdle_");
    if (instance) {
        instance->rxIdle();
    }
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::rxCompleted(std::size_t received) {
    // TODO: log somewhere a failure to allocate a new buffer
    Buffer::createI(Buffer::maxSize()) >=
    [&] (Buffer&& newBuffer) {
        return
        readMailbox->usedCountI() > 0 ?
        readMailbox->fetchI() >= [] (Buffer&& b) { b.releaseI(); return ok(boost::blank()); } : ok(boost::blank())
        >
        [&] () {
            readBuffer.resize(received);
            return
            readMailbox->postI(readBuffer) >
            [&]() {
                readBuffer = std::move(newBuffer);
                return ok(boost::blank());
            };
        };
    };
    uartStartReceiveI(driver.get(), Buffer::maxSize(), readBuffer.begin());
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::rxIdle() {
    std::size_t received = Buffer::maxSize() - dmaStreamGetTransactionSize(driver.get()->dmarx);
    if (received) {
        chSysLockFromISR();
        uartStopReceiveI(driver.get());
        rxCompleted(received);
        chSysUnlockFromISR();
    }
}

template <typename Buffer>
void SerialDeviceImpl<Buffer>::readTimeout_(void* p) {
    static_cast<This*>(p)->readTimeout();
}
