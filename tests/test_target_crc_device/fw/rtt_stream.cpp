#include "rtt_stream.h"

RTTStream rttStream{};
BaseSequentialStream* errorReportStream = &rttStream;
BaseSequentialStream* debugPrintStream = &rttStream;
