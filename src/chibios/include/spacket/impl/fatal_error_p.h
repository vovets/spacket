#include <spacket/fatal_error.h>

#include "osal.h"
#include "hal.h"


inline
void fatalError(const char* reason, const char* file, int line) {
    (void)file;
    (void)line;
    osalSysHalt(reason);
}
