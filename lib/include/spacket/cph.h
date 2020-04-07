#pragma once

#include <spacket/time_utils.h>
#include <spacket/memory_utils.h>
#include <spacket/guard_utils.h>

#include <boost/optional.hpp>

#include <functional>

namespace cph {

#ifdef CPH_ENABLE_DEBUG_PRINT

    IMPLEMENT_DPL_FUNCTION

#else

    IMPLEMENT_DPL_FUNCTION_NOP

#endif

}

template <typename Buffer>
using StreamT = boost::optional<Buffer>;

template <typename Buffer>
using HandlerT = std::function<Result<Void>(Buffer&&)>;

template <typename Buffer>
struct ConnectionT {
    using Stream = StreamT<Buffer>;
    using Handler = HandlerT<Buffer>;
    
    Stream* stream;
    Handler handler;
};

template <typename Stream>
typename Stream::value_type extract(Stream& stream) {
    using Buffer = typename Stream::value_type;
    if (!stream) { FATAL_ERROR("extract"); }
    Buffer tmp = std::move(*stream);
    stream = {};
    return tmp;
}

template <typename Array>
struct ArrayInserter {
    Array& array;
    std::size_t firstFree = 0;

    ArrayInserter(Array& array): array(array) {}
    
    void insert(typename Array::value_type v) {
        if (firstFree >= array.size()) {
            FATAL_ERROR("ArrayInserter::insertBack");
        }
        array[firstFree] = v;
        ++firstFree;
    }
};

template <typename Array>
ArrayInserter<Array> makeArrayInserter(Array& array) {
    return ArrayInserter<Array>(array);
}

template <typename Buffer>
struct UartT {
    using Uart = UartT<Buffer>;

    struct UARTConfigExt: UARTConfig {
        Uart& uart;

        UARTConfigExt(Uart& uart): uart(uart) {
            std::memset(static_cast<UARTConfig*>(this), 0, sizeof(UARTConfig));
        }
    };
    
private:
    UARTConfigExt uartConfig;
    UARTDriver& driver;
    boost::optional<Buffer> dmaRx;
    boost::optional<Buffer> dmaTx;
    thread_reference_t waitingThreadRx = nullptr;
    thread_reference_t waitingThreadTx = nullptr;
    std::size_t bytesReceived = 0;
    
public:
    UartT(UARTDriver& driver_)
        : uartConfig(*this)
        , driver(driver_)
    {
        uartConfig.txend2_cb = txEnd2_;
        uartConfig.rxend_cb = rxEnd_;
        uartConfig.rxerr_cb = rxError_;
        uartConfig.timeout_cb = rxIdle_;
        uartConfig.speed = 921600;
        uartConfig.cr1 = USART_CR1_IDLEIE;

        uartStart(&driver, &uartConfig);
    }

    void startRx(Buffer b) {
        uartStopReceive(&driver);
        dmaRx = std::move(b);
        bytesReceived = 0;
        uartStartReceive(&driver, dmaRx->size(),  dmaRx->begin());
    }

    Result<Buffer> waitRx(Timeout t) {
        auto guard = makeOsalSysLockGuard(); // dmaRx needs to be protected
        cph::dpl("waitRx|locked");
        if (!dmaRx) {
            return fail<Buffer>(toError(ErrorCode::UartNothingToWait));
        }
        if (driver.rxstate == UART_RX_ACTIVE) {
            cph::dpl("waitRx|suspend");
            auto result = osalThreadSuspendTimeoutS(&waitingThreadRx, toSystime(t));
            if (result == MSG_TIMEOUT) {
                return fail<Buffer>(toError(ErrorCode::UartRxTimeout));
            }
        }
        dmaRx->resize(bytesReceived);
        return extract(dmaRx);
    }

    void startTx(Buffer b) {
        uartStopSend(&driver);
        dmaTx = std::move(b);
        uartStartSend(&driver, dmaTx->size(), dmaTx->begin());
    }

    Result<Void> waitTx(Timeout t) {
        auto guard = makeOsalSysLockGuard();
        cph::dpl("waitTx|locked");
        if (driver.txstate == UART_TX_ACTIVE) {
            cph::dpl("waitTx|suspend");
            auto result = osalThreadSuspendTimeoutS(&waitingThreadTx, toSystime(t));
            if (result == MSG_TIMEOUT) {
                return fail(toError(ErrorCode::UartTxTimeout));
            }
        }
        return ok();
    }

private:
    void txEnd2I() {
        auto guard = makeOsalSysLockFromISRGuard();
        if (waitingThreadTx != nullptr) {
            osalThreadResumeI(&waitingThreadTx, MSG_OK);
        }
    }
    
    void rxEndI() {
        bytesReceived = dmaRx->size();
        rxFinishI();
    }
        
    void rxErrorI(uartflags_t) {
        // TODO: log error
        rxFinishI();
    }
    
    void rxIdleI() {
        rxFinishI();
    }

private:
    void rxFinishI() {
        auto guard = makeOsalSysLockFromISRGuard();
        stopReceiveI();
        if (bytesReceived > 0) {
            if (waitingThreadRx != nullptr) {
                osalThreadResumeI(&waitingThreadRx, MSG_OK);
            }
            return;
        }
        uartStartReceiveI(&driver, dmaRx->size(), dmaRx->begin());
    }

    void stopReceiveI() {
        if (driver.rxstate == UART_RX_ACTIVE) {
            auto notReceived = uartStopReceiveI(&driver);
            bytesReceived = dmaRx->size() - notReceived;
        }
    }
    
private:
    static Uart& getUart(UARTDriver* driver) {
        return static_cast<const UARTConfigExt*>(driver->config)->uart;
    }
    
    static void txEnd2_(UARTDriver* driver) {
        getUart(driver).txEnd2I();
    }

    static void rxEnd_(UARTDriver* driver) {
        getUart(driver).rxEndI();
    }

    static void rxError_(UARTDriver* driver, uartflags_t flags) {
        getUart(driver).rxErrorI(flags);
    }

    static void rxIdle_(UARTDriver* driver) {
        getUart(driver).rxIdleI();
    }
};

template <typename Buffer>
struct UartServiceT {
    using Stream = StreamT<Buffer>;
    using Uart = UartT<Buffer>;

    Stream rxInput;
    Stream up;
    Stream down;

    Uart& uart;

    UartServiceT(Uart& uart): uart(uart) {}

    template <typename ArrayInserterUp, typename ArrayInserterDown>
    void addHandlers(ArrayInserterUp& u, ArrayInserterDown& d) {
        u.insert({ &rxInput, [&](Buffer&& b){ return handleRxInput(std::move(b)); } });

        d.insert({ &down, [&](Buffer&& b){ return handleDown(std::move(b)); } });
    }

    Result<Void> handleRxInput(Buffer&& buffer) {
        cph::dpl("handleRxInput|enter");
        return
        Buffer::create(Buffer::maxSize()) >=
        [&](Buffer&& newBuffer) {
            uart.startRx(std::move(newBuffer));
            cph::dpl("handleRxInput|startRx");
            up = std::move(buffer);
            return ok();
        };
    }

    Result<Void> handleDown(Buffer&& buffer) {
        uart.startTx(std::move(buffer));
        cph::dpl("handleDown|startTx");
        return ok();
    }
};

template <typename Buffer>
struct ExecutorT {
    using Stream = StreamT<Buffer>;
    using Handler = HandlerT<Buffer>;
    using Connection = ConnectionT<Buffer>;
    
    template <std::size_t SIZE>
    using ConnArray = std::array<Connection, SIZE>;

    template <std::size_t NCONN>
    static Result<Void> execute(ConnArray<NCONN>& connections) {
        auto result = ok();
        for (const auto& c: connections) {
            if (!!*c.stream) {
                result = c.handler(extract(*c.stream));
                if (isFail(result)) { return result; }
            }
        }
        return ok();
    }
};
    
