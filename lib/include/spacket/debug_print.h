#pragma once

#include <spacket/debug_print_impl.h>

void debugPrint(const char *fmt, ...);
void debugPrintStart();
void debugPrintFinish();

void debugPrintLine(const char *fmt, ...);
