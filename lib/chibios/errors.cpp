#include <spacket/errors.h>
#include <spacket/buffer_utils.h>
#include <spacket/util/buffer_stream.h>

#include <hal.h>
#include <chprintf.h>

const char* toString(Error e, char* buffer, std::size_t size) {
    BufferView bv{reinterpret_cast<std::uint8_t*>(buffer), size};
    BufferSequentialStreamT<BufferView> ss{bv};
    int n = chprintf(&ss, "%d.%d", e.source, e.code);
    buffer[std::min(size - 1, static_cast<std::size_t>(std::max(0, n)))] = 0;
    return buffer;
}
