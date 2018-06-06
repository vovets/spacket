#include "rtt_stream.h"
#include "shell_commands.h"

#include "shell.h"
#include "chprintf.h"

#include <spacket/stm32/crc.h>

#include <boost/optional.hpp>

#include <cstdio>
#include <cinttypes>


boost::optional<stm32::Crc> gCrc;

#define ERROR(M) do { chprintf(stream, M"\r\n"); return; } while (false)

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


static void cmdAdd(BaseSequentialStream *stream, int numArgs, char* args[]) {
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
    {"add", cmdAdd},
    {"last", cmdLast},
    {NULL, NULL}
};

const ShellConfig shellConfig = {
    &rttStream,
    commands
};
