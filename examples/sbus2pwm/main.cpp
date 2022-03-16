#include "ch.h"
#include "hal.h"

#include "typedefs.h"
#include "sbus_decoder.h"

#include <spacket/buffer_debug.h>
#include <spacket/bottom_module.h>
#include <spacket/uart2.h>

#include <spacket/action.h>

namespace {

#ifdef ENABLE_DEBUG_PRINT
IMPLEMENT_DPL_FUNCTION
//IMPLEMENT_DPB_FUNCTION
#else
IMPLEMENT_DPL_FUNCTION_NOP
//IMPLEMENT_DPB_FUNCTION_NOP
#endif

} // namespace

struct BufferOut {
    Buffer& buffer;
    std::size_t writeIdx;
    const std::size_t size;

    BufferOut(Buffer& buffer): buffer(buffer), writeIdx(0), size(buffer.maxSize()) {}

    static void out(char c, void* arg) {
        static_cast<BufferOut*>(arg)->out_(c);
    }

    void out_(char c) {
        if (writeIdx < size) {
            buffer.begin()[writeIdx++] = c;
        }
    }

    void resizeBuffer() {
        buffer.resize(writeIdx);
    }
};

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

void updatePwm(const sbus::Packet& packet) {
    auto width = PWM_FRACTION_TO_WIDTH(
            pwmDriver,
            (sbusMax - sbusMin),
            norm(packet.channels[sbusChannel]));
    dpl("W %d", width);
    pwmEnableChannel(pwmDriver, pwmChannel, width);
}

Result<Buffer> printPacket(const sbus::Packet& packet, alloc::Allocator& allocator) {
    return
    Buffer::create(allocator) >=
    [&] (Buffer&& printBuffer) {
        BufferOut bo(printBuffer);
        oprintf(bo, "P %c %c", packet.frameLost ? 'L' : ' ', packet.failsafe ? 'F' : ' ');
        for (std::size_t i = 0; i < sbus::Packet::numChannels; ++i) {
            oprintf(bo, " %04d", packet.channels[i]);
        }
        oprintf(bo, "\r\n");
        bo.resizeBuffer();
        return ok(std::move(printBuffer));
    };
}

Storage<BufferAllocator> bufferAllocatorStorage;
Storage<FuncAllocator> funcAllocatorStorage;
Storage<TxtBufferAllocator> txtBufferAllocatorStorage;

int main() {
    halInit();
    chSysInit();
    SEGGER_RTT_Init();

    BufferAllocator& bufferAllocator = *new (&bufferAllocatorStorage) BufferAllocator;
    FuncAllocator& funcAllocator = *new (&funcAllocatorStorage) FuncAllocator;
    TxtBufferAllocator& txtBufferAllocator = *new (&txtBufferAllocatorStorage) TxtBufferAllocator;

    dbg::p("RTT ready\r\n");
    dbg::p("Buffer::maxSize(): %d\r\n", Buffer::maxSize(bufferAllocator));
    dbg::p("sizeof(Buffer): %d\r\n", sizeof(Buffer));
    dbg::p("sizeof(DeferredProc): %d\r\n", sizeof(DeferredProc));
    dbg::p("sizeof(Packet): %d\r\n", sizeof(sbus::Packet));

    Uart2 sbusUart(bufferAllocator, UARTD1);
    sbusUart
    .setBaud(100000)
    .setParity(Uart2::Parity::Even)
    .setStopBits(Uart2::StopBits::B_2);
    sbusUart.start();

    Uart2 monitorUart(bufferAllocator, UARTD2);
    monitorUart.setBaud(921600);
    monitorUart.start();

    Executor executor;
    Stack sbusStack;
    sbus::Decoder decoder(bufferAllocator, funcAllocator, executor);
    BottomModule sbusBottom(bufferAllocator, funcAllocator, executor, sbusUart);

    sbusStack.push(sbusBottom);
    sbusStack.push(decoder);

    Stack monitorStack;
    BottomModule monitorBottom(bufferAllocator, funcAllocator, executor, monitorUart);
    monitorStack.push(monitorBottom);

    sbusBottom.deferPoll() <= [&] (Error e) { logError(e); FATAL_ERROR("sbusBottom.deferPoll()"); return ok(); };
    monitorBottom.deferPoll() <= [&] (Error e) { logError(e); FATAL_ERROR("monitorBottom.deferPoll()"); return ok(); };

    PWMConfig pwmConfig = {};
    pwmConfig.frequency = STM32_TIMCLK1;
    pwmConfig.period = 0xFFFFU; // not 0x10000, so we can achieve true 100% pwm
    pwmConfig.channels[0].mode = PWM_OUTPUT_ACTIVE_HIGH;

    pwmStart(&PWMD1, &pwmConfig);

    auto startRead = [&](auto&& startReadRef) -> Result<Void> {
        return
        Module::ReadCompletionFunc::create(
            funcAllocator,
            [&](Result<Buffer>&& readResult) mutable {
                std::move(readResult) >=
                [&](Buffer&& b) {
                    cpm::dpb("main::startRead|read completion|", &b);
                    const sbus::Packet& packet = *reinterpret_cast<const sbus::Packet*>(b.begin());
                    updatePwm(packet);
                    return
                    printPacket(packet, txtBufferAllocator) >=
                    [&](Buffer&& txtBuffer) {
                        dbg::write(txtBuffer);
                        return
                        Module::WriteCompletionFunc::create(
                                funcAllocator,
                                [&](Result<Void>&& r) {
                                    cpm::dpl("main::startRead|write completion");
                                    return std::move(r) <= logError;
                                }
                        ) >=
                        [&](Module::WriteCompletionFunc&& writeCompletion) {
                            return
                            monitorBottom.write(std::move(txtBuffer), std::move(writeCompletion));
                        };
                    };
                } <= logError;
                return startReadRef(startReadRef);
            }
        ) >=
        [&](Module::ReadCompletionFunc&& completion) {
            return
            decoder.read(std::move(completion));
        };
    };

    startRead(startRead) <= logError;

    for (;;) {
        executor.executeOne() >= [](bool) { return ok(); } <= logError;
    }
}

extern "C"
void _putchar(char character) {
    (void)character;
    FATAL_ERROR("_putchar");
}
