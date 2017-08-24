#pragma once

#include <stdint.h>
#include <stddef.h>

extern "C" {

size_t stuff(const uint8_t* readPtr, size_t size, uint8_t* output);
size_t cobs_decode(const uint8_t * input, size_t length, uint8_t * output);

}
