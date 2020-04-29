#pragma once

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
        cpm::dpl("waitRx|locked");
        if (!dmaRx) {
            return fail<Buffer>(toError(ErrorCode::UartNothingToWait));
        }
        if (driver.rxstate == UART_RX_ACTIVE) {
            cpm::dpl("waitRx|suspend");
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
        cpm::dpl("waitTx|locked");
        if (driver.txstate == UART_TX_ACTIVE) {
            cpm::dpl("waitTx|suspend");
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
