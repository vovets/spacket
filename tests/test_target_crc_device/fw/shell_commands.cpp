#include "rtt_stream.h"
#include "shell_commands.h"

#include "shell.h"
#include "chprintf.h"

#include <spacket/stm32/crc.h>

#include <boost/optional.hpp>

#include <cstdio>
#include <cinttypes>


boost::optional<stm32::Crc> gCrc;

std::array<uint8_t, 16> gFromHexBuffer;

#define ERROR(M) do { chprintf(stream, M"\r\n"); return; } while (false)

static size_t readHex(BaseSequentialStream* stream, char* arg) {
    auto it = gFromHexBuffer.begin();
    for (; *arg != 0; ++arg, ++it) {
        if (it == gFromHexBuffer.end()) {
            chprintf(stream, "bad arg format, sequence too long (max %d digits)\r\n", 2 * gFromHexBuffer.size());
            return 0;
        }
        char* begin = arg;
        ++arg;
        if (*arg == 0) {
            chprintf(stream, "bad arg format, odd length sequence of hex digits\r\n");
            return 0;
        }
        uint16_t v;
        if (std::sscanf(begin, "%2" SCNx16, &v) != 1) {
            chprintf(stream, "bad arg format, should be sequence of hex digits\r\n");
            return 0;
        }
        if (v > 255) {
            chprintf(stream, "internal error\r\n");
            return 0;
        }
        *it = v;
    }
    return it - gFromHexBuffer.begin();
}

static void cmdReset(BaseSequentialStream* stream, int, char*[]) {
    chprintf(stream, "Bye!\r\n");
    NVIC_SystemReset();
}

static void cmdOpen(BaseSequentialStream* stream, int, char*[]) {
    if (gCrc) {
        ERROR("device is already opened");
    }
    gCrc = stm32::Crc::open(CRC);
}

static void cmdClose(BaseSequentialStream *, int, char*[]) {
    gCrc = boost::none;
}


static void cmdAddUint32(BaseSequentialStream *stream, int numArgs, char* args[]) {
    if (numArgs != 1) {
        ERROR("too few args, command needs a hex number arg");
    }
    uint32_t v;
    if (std::sscanf(args[0], "%10" SCNx32, &v) != 1) {
        ERROR("bad arg format, command needs a hex number arg");
    }
    if (!gCrc) {
        ERROR("device is not opened");
    }
    uint32_t l = gCrc->last();
    chprintf(stream, "last: %x\r\n", l);
    v = gCrc->add(v);
    chprintf(stream, "new: %x\r\n", v);
}

static void cmdAdd(BaseSequentialStream *stream, int numArgs, char* args[]) {
    if (numArgs != 1) {
        ERROR("too few args, command needs a hex byte sequence arg");
    }
    size_t readBytes = readHex(stream, args[0]);
    if (!readBytes) return;
    uint32_t l = gCrc->last();
    chprintf(stream, "last: %x\r\n", l);
    chprintf(stream, "buffer: ");
    for (auto b: gFromHexBuffer) {
        chprintf(stream, "%x", b);
    }
    chprintf(stream, "\r\n");
    uint32_t v = gCrc->add(&gFromHexBuffer[0], readBytes);
    chprintf(stream, "new: %x\r\n", v);
}

static void cmdLast(BaseSequentialStream *stream, int , char*[]) {
    if (!gCrc) {
        ERROR("device is not opened");
    }
    uint32_t l = gCrc->last();
    chprintf(stream, "last: %x\r\n", l);
}

#undef ERROR

static const ShellCommand commands[] = {
    {"reset", cmdReset},
    {"open", cmdOpen},
    {"close", cmdClose},
    {"add_uint32", cmdAddUint32},
    {"add", cmdAdd},
    {"last", cmdLast},
    {NULL, NULL}
};

const ShellConfig shellConfig = {
    &rttStream,
    commands
};
