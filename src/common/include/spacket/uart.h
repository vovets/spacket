#pragma once

#include <spacket/guard_utils.h>
#include <spacket/ring.h>
#include <spacket/driver.h>
#include <spacket/cpm_debug.h>


// TODO: design mechanism that allows for client to wait for completion
// of the actual transmission of all the bits from the shift register
// (use txend2 callback)

template <std::size_t RxRingCapacity, std::size_t TxRingCapacity>
struct UartT: Driver {
    using Uart = UartT<RxRingCapacity, TxRingCapacity>;
    using Queue = QueueT<Buffer>;
    using RxRing = RingT<Buffer, RxRingCapacity>;
    using TxRing = RingT<Buffer, TxRingCapacity>;

    enum class Parity {
        Even,
        Odd,
        None
    };

    enum class StopBits {
        B_0_5 = 0x1U,
        B_1 = 0x0U,
        B_1_5 = 0x3U,
        B_2 = 0x2U
    };

    struct UARTConfigExt: UARTConfig {
        Uart& uart;

        UARTConfigExt(Uart& uart): uart(uart) {
            std::memset(static_cast<UARTConfig*>(this), 0, sizeof(UARTConfig));
        }
    };

    struct RxRequestQueue: Queue {
        Uart& uart;

        RxRequestQueue(Uart& uart): uart(uart) {}

        // why is it needed?
        // google deleting destructors and https://eli.thegreenplace.net/2015/c-deleting-destructors-and-virtual-operator-delete/ 
        void operator delete(void*, std::size_t) {}

        bool canPut() const override {
            return uart.rxRequestCanPut();
        }

        bool put(Buffer& b) override {
            return uart.rxRequestPut(b);
        }
    };

    struct TxRequestQueue: Queue {
        Uart& uart;

        TxRequestQueue(Uart& uart): uart(uart) {}

        // why is it needed?
        // google deleting destructors and https://eli.thegreenplace.net/2015/c-deleting-destructors-and-virtual-operator-delete/ 
        void operator delete(void*, std::size_t) {}

        bool canPut() const override {
            return uart.txRequestCanPut();
        }

        bool put(Buffer& b) override {
            return uart.txRequestPut(b);
        }
    };
    
private:
    UARTConfigExt uartConfig;
    UARTDriver& driver;
    RxRing rxRing;
    TxRing txRing;
    RxRequestQueue rxRequestQueue_;
    TxRequestQueue txRequestQueue_;
    Queue* rxCompleteQueue = nullptr;
    Queue* txCompleteQueue = nullptr;
    std::size_t bytesReceived = 0;
    
public:
    UartT(UARTDriver& driver)
        : uartConfig(*this)
        , driver(driver)
        , rxRequestQueue_(*this)
        , txRequestQueue_(*this)
    {
        uartConfig.txend1_cb = txEnd1_;
        uartConfig.rxend_cb = rxEnd_;
        uartConfig.rxerr_cb = rxError_;
        uartConfig.timeout_cb = rxIdle_;
        uartConfig.speed = 921600;
        uartConfig.cr1 = USART_CR1_IDLEIE;
    }

    void setBaud(std::uint32_t baud) {
        uartConfig.speed = baud;
    }

    void setParity(Parity parity) {
        switch (parity) {
            case Parity::Even:
                uartConfig.cr1 &= ~USART_CR1_PS;
                uartConfig.cr1 |= USART_CR1_PCE | USART_CR1_M;
                break;
            case Parity::Odd:
                uartConfig.cr1 |= USART_CR1_PS;
                uartConfig.cr1 |= USART_CR1_PCE | USART_CR1_M;
                break;
            case Parity::None:
                uartConfig.cr1 &= ~(USART_CR1_PCE | USART_CR1_M);
        }
    }

    void setStopBits(StopBits stopBits) {
        uartConfig.cr2 = static_cast<uint16_t>(
            (uartConfig.cr2 & ~USART_CR2_STOP_Msk) |
            (static_cast<unsigned>(stopBits) << USART_CR2_STOP_Pos));
    }

    void start() {
        uartStart(&driver, &uartConfig);
    }

    void start(Queue& rxCompleteQueue_, Queue& txCompleteQueue_) override {
        rxCompleteQueue = &rxCompleteQueue_;
        txCompleteQueue = &txCompleteQueue_;
        start();
    }

    Queue& rxRequestQueue() override { return rxRequestQueue_; }
    Queue& txRequestQueue() override { return txRequestQueue_; }

    void stop() override {
        cpm::dpl("UartT::stop|rx");
        uartStopReceive(&driver);
        auto guard = makeOsalSysLockGuard();
        while (!rxRing.empty()) {
            auto tmp = std::move(rxRing.tail());
            rxRing.eraseTail();
            guard.unlock();
        }
        cpm::dpl("UartT::stop|tx");
        uartStopSend(&driver);
        guard = makeOsalSysLockGuard();
        while (!txRing.empty()) {
            auto tmp = std::move(txRing.tail());
            txRing.eraseTail();
            guard.unlock();
        }
    }
    
    void lock() override {
        osalSysLock();
    }

    void unlock() override {
        osalSysUnlock();
    }
    
private:
    bool rxRequestCanPut() const {
        return !rxRing.full();
    }
    
    bool rxRequestPut(Buffer& b) {
        if (rxRing.full()) { return false; }
        cpm::dpb("UartT::rxRequestPut|", &b);
        bool wasEmpty = rxRing.empty();
        if (!rxRing.put(b)) { return false; }
        if (wasEmpty) {
            auto& tail = rxRing.tail();
            bytesReceived = 0;
            cpm::dpb("UartT::rxRequestPut|uartStartReceive ", &tail);
            uartStartReceiveI(&driver, tail.size(), tail.begin());
        }
        return true;
    }

    bool txRequestCanPut() const {
        return !txRing.full();
    }

    bool txRequestPut(Buffer& b) {
        cpm::dpb("UartT::txRequestPut|", &b);
        bool wasEmpty = txRing.empty();
        if (!txRing.put(b)) { return false; }
        if (wasEmpty) {
            auto& tail = txRing.tail();
            cpm::dpl("UartT::txRequestPut|uartStartSend");
            uartStartSendI(&driver, tail.size(), tail.begin());
        }
        return true;
    }
    
private:
    void rxFinishI() {
        cpm::dpl("UartT::rxFinishI|");
        auto guard = makeOsalSysLockFromISRGuard();
        if (rxCompleteQueue == nullptr) { FATAL_ERROR("UartT::rxFinishI|rxCompleteQueue == nullptr"); }

        if (driver.rxstate != UART_RX_ACTIVE && driver.rxstate != UART_RX_COMPLETE) {
            cpm::dpl("UartT::rxFinishI|idle");
            return;
        }

        // it should not be empty here, but anyway
        // (because uartStartReceive invoked only if the ring was not empty)
        if (rxRing.empty()) {
            cpm::dpl("UartT::rxFinishI|rxRing empty");
            return;
        }

        auto notReceived = uartStopReceiveI(&driver);
        bytesReceived = rxRing.tail().size() - notReceived;
        cpm::dpl("UartT::rxFinishI|bytesReceived=%u", bytesReceived);
        
        if (bytesReceived == 0) {
            uartStartReceiveI(&driver, rxRing.tail().size(), rxRing.tail().begin());
            return;
        }
        
        if (!rxCompleteQueue->canPut()) {
            cpm::dpl("UartT::rxFinishI|rxCompleteQueue blocked");
            // TODO: log lost packet
            bytesReceived = 0;
            uartStartReceiveI(&driver, rxRing.tail().size(), rxRing.tail().begin());
            return;
        }
        
        rxRing.tail().resize(bytesReceived);
        if (!rxCompleteQueue->put(rxRing.tail())) { FATAL_ERROR("UartT::rxFinishI|rxRing full"); }
        rxRing.eraseTail();
        
        if (!rxRing.empty()) {
            bytesReceived = 0;
            uartStartReceiveI(&driver, rxRing.tail().size(), rxRing.tail().begin());
        }
    }
    
    void txFinishI() {
        cpm::dpl("UartT::txFinishI|");
        auto guard = makeOsalSysLockFromISRGuard();
        if (txCompleteQueue == nullptr) { FATAL_ERROR("UartT::txFinishI|txCompleteQueue == nullptr"); }

        // it should not be empty here, but anyway
        // (because uartStartSend invoked only if the ring was not empty)
        if (txRing.empty()) {
            cpm::dpl("UartT::txFinishI|txRing empty");
            return;
        }

        auto bytesNotSent = uartStopSendI(&driver);
        cpm::dpl("UartT::txFinishI|bytesNotSent=%u", bytesNotSent);
        
        if (!txCompleteQueue->canPut()) {
            FATAL_ERROR("UartT::txFinishI|txCompleteQueue blocked");
            return;
        }
        
        if (!txCompleteQueue->put(txRing.tail())) {
            FATAL_ERROR("UartT::txFinishI|txCompleteQueue blocked");
        }
        
        txRing.eraseTail();
        
        if (!txRing.empty()) {
            auto& tail = txRing.tail();
            cpm::dpl("UartT::txFinishI|uartStartSendI|%X", tail.id());
            uartStartSendI(&driver, tail.size(), tail.begin());
        }
    }
    
private:
    void txEnd1I() {
        cpm::dpl("UartT::txEndI1|");
        txFinishI();
    }
    
    void rxEndI() {
        cpm::dpl("UartT::rxEndI|");
        rxFinishI();
    }
        
    void rxErrorI(uartflags_t flags) {
        // TODO: log error
        cpm::dpl("UartT::rxErrorI|flags=0x%X", flags);
//        debugPrintStart();
//        debugPrint("UartT::rxErrorI|");
//        if (flags & UART_PARITY_ERROR)   debugPrint("PAR "); else debugPrint("    ");
//        if (flags & UART_FRAMING_ERROR)  debugPrint("FRM "); else debugPrint("    ");
//        if (flags & UART_OVERRUN_ERROR)  debugPrint("OVR "); else debugPrint("    ");
//        if (flags & UART_NOISE_ERROR)    debugPrint("NSE "); else debugPrint("    ");
//        if (flags & UART_BREAK_DETECTED) debugPrint("BRK");  else debugPrint("   ");
//        debugPrintFinish();

        // ignore break detection
        // why USART_SR_LBD gets set if USART_CR2_LINEN is 0 ?
        if (flags & UART_BREAK_DETECTED) return;
        rxFinishI();
    }
    
    void rxIdleI() {
        cpm::dpl("UartT::rxIdleI|");
        rxFinishI();
    }

private:
    static Uart& getUart(UARTDriver* driver) {
        return static_cast<const UARTConfigExt*>(driver->config)->uart;
    }
    
    static void txEnd1_(UARTDriver* driver) {
        getUart(driver).txEnd1I();
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
