#pragma once

#include <spacket/result.h>
#include <spacket/buffer.h>
#include <spacket/time_utils.h>

#include <tuple>

using Source = std::function<Result<Buffer>(Timeout t, size_t maxSize)>;

Result<Buffer> readSync(Source s, size_t maxRead, Timeout t);

Result<std::tuple<Buffer, Buffer>> readPacket(Source s, Timeout t);
