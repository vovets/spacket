#pragma once

#include <spacket/impl/check_stack_p.h>


#ifdef SPACKET_ENABLE_CHECK_STACK
#define SPACKET_CHECK_STACK() check_stack_impl::checkStack()
#else
#define SPACKET_CHECK_STACK() do {} while (false)
#endif
