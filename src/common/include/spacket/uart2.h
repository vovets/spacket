#pragma once

#include <spacket/buffer.h>
#include <spacket/buffer_box.h>
#include <spacket/allocator.h>
#include <spacket/cpm_debug.h>
#include <spacket/guard_utils.h>
#include <spacket/special_ring.h>
#include <spacket/driver.h>

#include <hal.h>

template <std::size_t RX_RING_CAPACITY, std::size_t TX_RING_CAPACITY>
class Uart2T: public Driver2 {
    
    struct UARTConfigExt: UARTConfig {
        Uart2T& uart;

        UARTConfigExt(Uart2T& uart): uart(uart) {
            std::memset(static_cast<UARTConfig*>(this), 0, sizeof(UARTConfig));
        }
    };

    using RxRing = SpecialRingT<Buffer, RX_RING_CAPACITY>;
    using TxRing = SpecialRingT<Buffer, TX_RING_CAPACITY>;

    alloc::Allocator& allocator;
    UARTDriver& driver;
    UARTConfigExt uartConfig;
    RxRing rxRing;
    TxRing txRing;
    

private:
    void lock() {
        osalSysLock();
    }

    void unlock() {
        osalSysUnlock();
    }

    auto lockGuard() {
        return makeGuard([=]() { this->lock(); }, [&](){ this->unlock(); });
    }

    void deallocateTx() {
        for (;;) {
            auto guard = lockGuard();
            if (txRing.canGet()) {
                Buffer tmp(txRing.get());
                guard.unlock();
            } else {
                break;
            }
        }
    }

    Result<Void> allocateRx() {
        if (!rxRing.canPut()) { return ok(); }
        cpm::dpl("Uart2T::allocateRx|allocating");
        for (;;) {
            auto r = Buffer::create(allocator);
            if (isFail(r)) { return fail(getFailUnsafe(r)); }
            auto guard = lockGuard();
            if (!rxRing.canPut()) { return ok(); }
            rxRing.put(getOkUnsafe(std::move(r)));
        }
    }

    void rxBufferLost() {
        cpm::dpl("Uart2T::rxBufferLost");
    }
    
    void txStalled() {
        cpm::dpl("Uart2T::txStalled");
    }

    void startRxI() {
        cpm::dpb("Uart2T::startRxI|", rxRing.point());
        uartStartReceiveI(&driver, rxRing.point()->size(), rxRing.point()->begin());
    }

    std::size_t stopRxI() {
        cpm::dpl("Uart2T::stopRxI|");
        return uartStopReceiveI(&driver);
    }

    std::size_t stopTxI() {
        return uartStopSendI(&driver);
    }
    
    void startRx() {
        auto guard = lockGuard();
        if (driver.state != UART_READY) { return; }
        if (driver.rxstate != UART_RX_IDLE) { return; }
        if (rxRing.point() == nullptr) { return; }
        startRxI();
    }

    void startTxI() {
        cpm::dpb("Uart2T::startTxI|", txRing.point());
        uartStartSendI(&driver, txRing.point()->size(), txRing.point()->begin());
    }

    void startTx() {
        auto guard = lockGuard();
        if (driver.state != UART_READY) { return; }
        if (driver.txstate != UART_TX_IDLE) { return; }
        if (txRing.point() == nullptr) { return; }
        startTxI();
    }

    void rxFinishI() {
        auto guard = makeOsalSysLockFromISRGuard();

        if (driver.rxstate == UART_RX_IDLE) {
            cpm::dpl("Uart2T::rxFinishI|idle");
            return;
        }


        auto left = stopRxI();
        auto received = rxRing.point()->size() - left;
        cpm::dpl("Uart2T::rxFinishI|received=%d", received);
        
        if (received == 0) {
            startRxI();
            return;
        }

        rxRing.point()->resize(received);
        rxRing.advancePoint();
        if (rxRing.point() != nullptr) {
            startRxI();
        }
    }

    void txFinishI() {
        auto guard = makeOsalSysLockFromISRGuard();

        if (driver.txstate == UART_TX_IDLE) {
            return;
        }

        txRing.advancePoint();
        if (txRing.point() != nullptr) {
            startTxI();
        }
    }
    
public:
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

    Uart2T(alloc::Allocator& allocator, UARTDriver& driver):
        allocator(allocator)
        , driver(driver)
        , uartConfig(*this) {
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

    void start() override {
        uartStart(&driver, &uartConfig);
    }

    Result<Void> serviceRx() override {
        return
        allocateRx() >
        [&]() {
            startRx();
            return ok();
        };
    }

    bool rxReady() override {
        return rxRing.canGet();
    }

    Result<Buffer> rx() override {
        auto guard = lockGuard();
        if (rxRing.canGet()) {
            return ok(rxRing.get());
        }
        return fail<Buffer>(toError(ErrorCode::Driver2RxNotReady));
    }
    
    Result<Void> serviceTx() override {
        deallocateTx();
        startTx();
        return ok();
    }

    bool txReady() override {
        return txRing.canPut();
    }

    Result<Void> tx(Buffer&& b) override {
        {
            auto guard = lockGuard();
            if (txRing.canPut()) {
                txRing.put(std::move(b));
                guard.unlock();
                startTx();
                return ok();
            }
        }
        return fail(toError(ErrorCode::Driver2TxNotReady));
    }

private:
    void txEnd1I() {
        cpm::dpl("Uart2T::txEnd1I|");
        txFinishI();
    }
    
    void rxEndI() {
        cpm::dpl("Uart2T::rxEndI|");
        rxFinishI();
    }
        
    void rxErrorI(uartflags_t flags) {
        // TODO: log error
        cpm::dpl("Uart2T::rxErrorI|flags=0x%X", flags);
//         debugPrintStart();
//         debugPrint("Uart2T::rxErrorI|");
//         if (flags & UART_PARITY_ERROR)   debugPrint("PAR "); else debugPrint("    ");
//         if (flags & UART_FRAMING_ERROR)  debugPrint("FRM "); else debugPrint("    ");
//         if (flags & UART_OVERRUN_ERROR)  debugPrint("OVR "); else debugPrint("    ");
//         if (flags & UART_NOISE_ERROR)    debugPrint("NSE "); else debugPrint("    ");
//         if (flags & UART_BREAK_DETECTED) debugPrint("BRK");  else debugPrint("   ");
//         debugPrintFinish();

        // ignore break detection
        // why USART_SR_LBD gets set if USART_CR2_LINEN is 0 ?
        if (flags & UART_BREAK_DETECTED) return;
        rxFinishI();
    }
    
    void rxIdleI() {
        cpm::dpl("Uart2T::rxIdleI|");
        rxFinishI();
    }

private:
    static Uart2T& getUart(UARTDriver* driver) {
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
