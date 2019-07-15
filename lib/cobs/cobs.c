#include <stdint.h>
#include <stddef.h>


/* Returns the number of bytes written to "output".
 * Output must be at least length + length / 254 + 1 bytes long.
 *
 */
size_t stuff(const uint8_t * restrict readPtr, size_t size, uint8_t * restrict output) {
    size_t chunkRead = 0;
    uint8_t* writePtr = output;
    uint8_t* codePtr = output;
    const uint8_t* end = readPtr + size;
    for (; readPtr < end; ++readPtr) {
        if (chunkRead == 0) {
            if (*readPtr == 0) {
                *writePtr++ = 1;
            } else {
                codePtr = writePtr++;
                *writePtr++ = *readPtr;
                chunkRead = 1;
            }
        } else {
            if (*readPtr == 0) {
                *codePtr = chunkRead + 1;
                chunkRead = 0;
            } else {
                *writePtr++ = *readPtr;
                ++chunkRead;
                if (chunkRead == 254) {
                    *codePtr = 255;
                    chunkRead = 0;
                    codePtr = writePtr;
                }
            }
        }
    }
    if (chunkRead != 0) { // means that input data doesn't end with 0, i.e. end occurred in the middle of reading a chunk
        *codePtr = chunkRead + 1;
    } else {
        if (*(readPtr - 1) == 0) { // do not append 0 if packet ends with 255-code block
            *writePtr++ = 1;
        }
    }
    return writePtr - output;
}

/* Returns the number of bytes written to "output" if
 * buffer was successfully unstuffed, and 0 if there was an
 * error unstuffing buffer.
 * No zero bytes allowed in buffer pointed to by "readPtr".
 */
size_t unstuff(const uint8_t* restrict readPtr, size_t size, uint8_t* restrict output)
{
    const uint8_t* end = readPtr + size;
    uint8_t* writePtr = output;
    while (readPtr < end) {
        uint8_t code = *readPtr;
        ++readPtr;
        for (uint8_t toCopy = code - 1; toCopy; --toCopy) {
            if (readPtr >= end) { return 0; }
            *writePtr++ = *readPtr++;
        }
        if (readPtr < end && code != 255) {
            *writePtr++ = 0;
        }
    }
    return writePtr - output;
}
