#pragma once

#include <spacket/buffer.h>

#include <boost/optional.hpp>

using BufferBox = std::optional<Buffer>;

Buffer extract(BufferBox& bb) {
    Buffer retval = std::move(*bb);
    bb = {};
    return retval;
}
