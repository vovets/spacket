#include <stdint.h>
#include <stddef.h>


/* Returns the number of bytes written to "output".
 * Output must be at least length + length / 254 bytes long.
 *
 */
size_t stuff(const uint8_t * restrict readPtr, size_t size, uint8_t * restrict output) {
    size_t chunkWritten = 0;
    uint8_t* writePtr = output;
    uint8_t* codePtr = output;
    const uint8_t* end = readPtr + size;
    for (; readPtr < end; ++readPtr) {
        if (!chunkWritten) {
            if (*readPtr == 0) {
                *writePtr++ = 1;
            } else {
                codePtr = writePtr++;
                *writePtr++ = *readPtr;
                chunkWritten = 2;
            }
        } else {
            if (*readPtr == 0) {
                *codePtr = chunkWritten;
                chunkWritten = 0;
            } else {
                *writePtr++ = *readPtr;
                ++chunkWritten;
            }
        }
    }
    if (chunkWritten) {
        *codePtr = chunkWritten;
    }
    return writePtr - output;
}

/* Unstuffs "length" bytes of data at the location pointed to by
 * "input", writing the output * to the location pointed to by
 * "output". Returns the number of bytes written to "output" if
 * "input" was successfully unstuffed, and 0 if there was an
 * error unstuffing "input".
 *
 * Remove the "restrict" qualifiers if compiling with a
 * pre-C99 C dialect.
 */
size_t cobs_decode(const uint8_t * restrict input, size_t length, uint8_t * restrict output)
{
    size_t read_index = 0;
    size_t write_index = 0;
    uint8_t code;
    uint8_t i;

    while(read_index < length)
    {
        code = input[read_index];

        if(read_index + code >= length && code != 1)
        {
            return 0;
        }

        ++read_index;

        for(i = 1; i < code; i++)
        {
            output[write_index++] = input[read_index++];
        }
        if((code != 0xFF && read_index != length) || code == 1)
        {
            output[write_index++] = '\0';
        }
    }

    return write_index;
}
