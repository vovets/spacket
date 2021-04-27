#include "rtt_stream.h"

RTTStream rttStream{};
BaseSequentialStream* errorStream = &rttStream;
BaseSequentialStream* debugPrintStream = &rttStream;
