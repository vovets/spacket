#pragma once

#include <spacket/debug_print.h>
#include <spacket/buffer_debug.h>

namespace cpm {

#ifdef CPH_ENABLE_DEBUG_PRINT

IMPLEMENT_DPL_FUNCTION
IMPLEMENT_DPB_FUNCTION

#else

IMPLEMENT_DPL_FUNCTION_NOP
IMPLEMENT_DPB_FUNCTION_NOP

#endif

}
