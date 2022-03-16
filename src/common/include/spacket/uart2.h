#pragma once

#include <spacket/buffer.h>
#include <spacket/buffer_box.h>
#include <spacket/allocator.h>
#include <spacket/cpm_debug.h>
#include <spacket/guard_utils.h>
#include <spacket/special_ring.h>
#include <spacket/driver.h>

#include <hal.h>

namespace impl {

template <typename Enum>
auto toInt(bool condition, Enum e) -> std::underlying_type_t<Enum> {
    constexpr std::underlying_type_t<Enum> zero = 0;
    return condition ? static_cast<std::underlying_type_t<Enum>>(e) : zero;
}

} // impl

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

public:
    enum class Parity {
        Even,
        Odd,
        None
    };

    enum class StopBits {
        B_0_5 = USART_CR2_STOP_0,
        B_1 = 0x0U,
        B_1_5 = USART_CR2_STOP_0 | USART_CR2_STOP_1,
        B_2 = USART_CR2_STOP_1
    };

    Uart2T(alloc::Allocator& allocator, UARTDriver& driver):
            allocator(allocator)
            , driver(driver)
            , uartConfig(*this) {
        uartConfig.txend1_cb = txEnd1_;
        uartConfig.rxend_cb = rxEnd_;
        uartConfig.rxerr_cb = rxError_;
        uartConfig.timeout_cb = rxIdle_;
        uartConfig.speed = 115200;
        uartConfig.cr1 = USART_CR1_IDLEIE;
    }

    Uart2T& setBaud(std::uint32_t baud) {
        uartConfig.speed = baud;
        return *this;
    }

    Uart2T& setParity(Parity parity) {
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
        return *this;
    }

    Uart2T& setStopBits(StopBits stopBits) {
        uartConfig.cr2 = static_cast<uint16_t>(
                (uartConfig.cr2 & ~USART_CR2_STOP) | static_cast<unsigned>(stopBits));
        return *this;
    }

    void start() override {
        uartStart(&driver, &uartConfig);
    }

    Result<Void> serviceRx() override {
//        cpm::dpl("Uart2T::serviceRx|");
        return
                allocateRx() >
                [&]() {
                    startRx();
                    return ok();
                };
    }

    Events poll() override {
        return Events {
            static_cast<std::uint8_t>(
            impl::toInt(  rxRing.canGet(), Event::RX_READY)
            | impl::toInt(txRing.canPut(), Event::TX_READY)
            | impl::toInt(rxRing.canPut(), Event::RX_NEEDS_SERVICE)
            | impl::toInt(txRing.canGet(), Event::TX_NEEDS_SERVICE))
        };
    }

    Result<Buffer> rx() override {
        auto guard = lockGuard();
        if (rxRing.canGet()) {
            Buffer b = rxRing.get();
            cpm::dpb("Uart2T::rx|", &b);
            return ok(std::move(b));
        }
        cpm::dpl("Uart2T::rx|RxNotReady");
        return fail<Buffer>(toError(ErrorCode::Driver2RxNotReady));
    }

    Result<Void> serviceTx() override {
        deallocateTx();
        startTx();
        return ok();
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
                cpm::dpl("Uart2T::deallocateTx|deallocating");
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

    void rxStalled() {
        cpm::dpl("Uart2T::rxStalled");
    }
    
    void txStalled() {
        cpm::dpl("Uart2T::txStalled");
    }

    void startRxI() {
//        cpm::dpl("Uart2T::startRxI|enter");
        cpm::dpb("Uart2T::startRxI|", rxRing.point());
        uartStartReceiveI(&driver, rxRing.point()->size(), rxRing.point()->begin());
//        cpm::dpl("Uart2T::startRxI|exit");
    }

    std::size_t stopRxI() {
        std::size_t n = uartStopReceiveI(&driver);
        cpm::dpl("Uart2T::stopRxI|n=%d", n);
        return n;
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
        cpm::dpl("Uart2T::rxFinishI|left=%d", left);
        if (rxRing.point() == nullptr) { FATAL_ERROR("Uart2T::rxFinishI|rxRing point is null"); }

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
            return;
        }
        rxStalled();
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
private:
    void txEnd1I() {
        cpm::dpl("Uart2T::txEnd1I|");
        txFinishI();
    }
    
    void rxEndI() {
        cpm::dpl("Uart2T::rxEndI|");
        rxFinishI();
    }

    void debugPrintFlags(uartflags_t flags) {
        dbg::ArrayOut out;
        dbg::prefix(out);
        oprintf(out, "Uart2T::rxErrorI|");
        if (flags & UART_PARITY_ERROR)   oprintf(out, "PAR "); else oprintf(out, "    ");
        if (flags & UART_FRAMING_ERROR)  oprintf(out, "FRM "); else oprintf(out, "    ");
        if (flags & UART_OVERRUN_ERROR)  oprintf(out, "OVR "); else oprintf(out, "    ");
        if (flags & UART_NOISE_ERROR)    oprintf(out, "NSE "); else oprintf(out, "    ");
        if (flags & UART_BREAK_DETECTED) oprintf(out, "BRK");  else oprintf(out, "   ");
        dbg::suffix(out);
        dbg::write(out);
    }
    void rxErrorI(uartflags_t flags) {
        // TODO: log error
//        cpm::dpl("Uart2T::rxErrorI|flags=0x%X", flags);
        // ignore break detection
        // why USART_SR_LBD gets set if USART_CR2_LINEN is 0 ?
        flags &= ~UART_BREAK_DETECTED;
        if (flags == 0) return;
        //debugPrintFlags(flags);
        rxFinishI();
    }
    
    void rxIdleI() {
//        cpm::dpl("Uart2T::rxIdleI|");
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
