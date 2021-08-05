#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "typedefs.h"
#include "sbus_decoder.h"

#include <spacket/buffer_debug.h>
#include <spacket/bottom_module.h>
#include <spacket/uart2.h>


namespace {

#ifdef ENABLE_DEBUG_PRINT
IMPLEMENT_DPL_FUNCTION
//IMPLEMENT_DPB_FUNCTION
#else
IMPLEMENT_DPL_FUNCTION_NOP
//IMPLEMENT_DPB_FUNCTION_NOP
#endif

} // namespace

using ChannelValue = sbus::Packet::ChannelValue;

auto* pwmDriver = &PWMD1;
constexpr pwmchannel_t pwmChannel = 0;
constexpr std::size_t sbusChannel = 3;
constexpr ChannelValue sbusMin = 352;
constexpr ChannelValue sbusMax = 1696;

ChannelValue norm(ChannelValue value) {
    value = std::max(value, sbusMin);
    value = std::min(value, sbusMax);
    return value - sbusMin;
}

void process(const Buffer& buffer) {
    const sbus::Packet* packet = reinterpret_cast<const sbus::Packet*>(buffer.begin());
    debugPrint("P %c %c", packet->frameLost ? 'L' : ' ', packet->failsafe ? 'F' : ' ');
    for (std::size_t i = 0; i < sbus::Packet::numChannels; ++i) {
        debugPrint(" %04d", packet->channels[i]);
    }
    debugPrintFinish();
    auto width = PWM_FRACTION_TO_WIDTH(
        pwmDriver,
        (sbusMax - sbusMin),
        norm(packet->channels[sbusChannel]));
    dpl("W %d", width);
    pwmEnableChannel(pwmDriver, pwmChannel, width);
}

Allocator allocator;

struct Top: Module {
    Result<Void> up(Buffer&& b) override {
        cpm::dpl("Bottom::up|");
        process(b);
        return ok();
    }
    
    Result<Void> down(Buffer&&) override {
        cpm::dpl("Top::down|");
        return ok();
    }
};

int main() {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize(allocator));
    chprintf(&rttStream, "sizeof(Buffer): %d\r\n", sizeof(Buffer));
    chprintf(&rttStream, "sizeof(DeferredProc): %d\r\n", sizeof(DeferredProc));
    chprintf(&rttStream, "sizeof(Packet): %d\r\n", sizeof(sbus::Packet));

    Uart2 uart(allocator, UARTD1);
    uart.setBaud(100000);
    uart.setParity(Uart2::Parity::Even);
    uart.setStopBits(Uart2::StopBits::B_2);
    uart.start();

    Executor executor;
    Stack stack(executor);
    sbus::Decoder decoder(allocator);
    BottomModule bottom(uart);
    Top top;

    stack.push(bottom);
    stack.push(decoder);
    stack.push(top);

    PWMConfig pwmConfig = {};
    pwmConfig.frequency = 72000000;
    pwmConfig.period = 0xFFFFU; // not 0x10000, so we can achieve true 100% pwm
    pwmConfig.channels[0].mode = PWM_OUTPUT_ACTIVE_HIGH;

    pwmStart(&PWMD1, &pwmConfig);

    for (;;) {
        bottom.service(executor) <= logError;
    }
}
