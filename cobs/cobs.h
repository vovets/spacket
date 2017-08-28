#pragma once

#include <stdint.h>
#include <stddef.h>

extern "C" {

size_t stuff(const uint8_t* readPtr, size_t size, uint8_t* output);
size_t unstuff(const uint8_t* readPtr, size_t size, uint8_t * output);

}
