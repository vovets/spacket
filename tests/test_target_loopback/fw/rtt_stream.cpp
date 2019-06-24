#include "rtt_stream.h"

RTTStream rttStream{};
BaseSequentialStream* debugPrintStream = &rttStream;
BaseSequentialStream* errorReportStream = &rttStream;