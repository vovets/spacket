#pragma once

void fatalError(const char* reason, const char* file, int line);

#define FATAL_ERROR(reason) fatalError(reason, __FILE__, __LINE__)
