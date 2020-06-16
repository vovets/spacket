#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtt_stream.h"
#include "typedefs.h"
#include "sbus_decoder.h"

#include <spacket/buffer_debug.h>


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

int main() {
    halInit();
    chSysInit();

    chprintf(&rttStream, "RTT ready\r\n");
    chprintf(&rttStream, "Buffer::maxSize(): %d\r\n", Buffer::maxSize());
    chprintf(&rttStream, "sizeof(Buffer): %d\r\n", sizeof(Buffer));
    chprintf(&rttStream, "sizeof(DeferredProc): %d\r\n", sizeof(DeferredProcT<Buffer>));
    chprintf(&rttStream, "sizeof(Packet): %d\r\n", sizeof(sbus::Packet));

    Uart uart(UARTD1);
    uart.setBaud(100000);
    uart.setParity(Uart::Parity::Even);
    uart.setStopBits(Uart::StopBits::B_2);

    Stack stack(uart);
    sbus::Decoder decoder;
    Endpoint endpoint;
    
    stack.push(decoder);
    stack.push(endpoint);

    PWMConfig pwmConfig = {};
    pwmConfig.frequency = 72000000;
    pwmConfig.period = 0xFFFFU; // not 0x10000, so we can achieve true 100% pwm
    pwmConfig.channels[0].mode = PWM_OUTPUT_ACTIVE_HIGH;

    pwmStart(&PWMD1, &pwmConfig);

    for (;;) {
        stack.tick();
        endpoint.read() >=
        [&] (Buffer&& b) {
            process(b);
            return ok();
        } <=
        [] (Error e) {
            if (e != toError(ErrorCode::Timeout)) {
                return threadErrorReport(e);
            }
            return fail(e);
        };
    }
}