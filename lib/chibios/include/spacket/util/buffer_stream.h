#pragma once

#include <hal.h>

#include <spacket/fatal_error.h>

template <typename Buffer>
struct BufferSequentialStreamT: BaseSequentialStream {
    static BufferSequentialStreamT* cast(void* p) { return static_cast<BufferSequentialStreamT*>(p); }

    static size_t write(void *instance, const uint8_t *bp, size_t n) {
        return cast(instance)->write(bp, n);
    }

    static size_t read(void *instance, uint8_t *bp, size_t n) {
        return cast(instance)->read(bp, n);
    }

    static msg_t put(void *instance, uint8_t b) {
        return cast(instance)->put(b);
    }

    static msg_t get(void *instance) {
        return cast(instance)->get();
    }

    static const struct BaseSequentialStreamVMT vmt_;

    Buffer& buffer;
    std::size_t pos;

    BufferSequentialStreamT(Buffer& buffer): BaseSequentialStream{&vmt_}, buffer(buffer), pos(0) {}

    size_t write(const uint8_t*, size_t) {
        FATAL_ERROR("not implemented");
        return 0;
    }

    size_t read(uint8_t*, size_t) {
        FATAL_ERROR("not implemented");
        return 0;
    }

    msg_t put(uint8_t b) {
        if (pos < buffer.size()) {
            buffer.begin()[pos++] = b;
            return MSG_OK;
        }
        return MSG_RESET;
    }

    msg_t get() {
        FATAL_ERROR("not implemented");
        return MSG_RESET;
    }
};

template <typename Buffer>
const struct BaseSequentialStreamVMT BufferSequentialStreamT<Buffer>::vmt_ =
{
&BufferSequentialStreamT<Buffer>::write,
&BufferSequentialStreamT<Buffer>::read,
&BufferSequentialStreamT<Buffer>::put,
&BufferSequentialStreamT<Buffer>::get
};
