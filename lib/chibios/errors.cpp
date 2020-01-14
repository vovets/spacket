#include <spacket/errors.h>
#include <spacket/buffer_utils.h>
#include <spacket/util/buffer_stream.h>

#include <hal.h>
#include <chprintf.h>

const char* toString(Error e, char* buffer, std::size_t size) {
    BufferView bv{reinterpret_cast<std::uint8_t*>(buffer), size};
    BufferSequentialStreamT<BufferView> ss{bv};
    chprintf(&ss, "%d.%d", e.source, e.code);
    return buffer;
}
