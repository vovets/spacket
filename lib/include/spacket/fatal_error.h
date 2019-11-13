#pragma once

void fatalError(const char* reason, const char* file, int line);

#ifdef ENABLE_FILE_LINE_INFO
#define FATAL_ERROR(reason) fatalError(reason, __FILE__, __LINE__)
#else
#define FATAL_ERROR(reason) fatalError(reason, nullptr, 0)
#endif
