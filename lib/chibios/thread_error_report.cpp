#include <spacket/util/thread_error_report.h>

#include "chprintf.h"

Result<boost::blank> threadErrorReport(Error e) {
    chprintf(errorReportStream, "E:%s: %s\r\n", chRegGetThreadNameX(chThdGetSelfX()), toString(e));
    return ok(boost::blank{});
}
