#pragma once

#include <spacket/stm32/crc_impl.h>

#include <utility>

namespace stm32 {

class Crc: private CrcImpl {
    friend class CrcImpl;
public:
    template <typename... U>
    static Crc open(U&&... u) {
        return CrcImpl::open<Crc>(std::forward<U>(u)...);
    }

    Crc(Crc&& src) noexcept: CrcImpl(std::move(src)) {}

    Crc& operator=(Crc&& src) noexcept {
        *static_cast<CrcImpl*>(this) = std::move(src);
        return *this;
    }

    uint32_t add(uint32_t v) {
        return CrcImpl::add(v);
    }

    uint32_t last() {
        return CrcImpl::last();
    }

private:
    Crc(CrcImpl&& impl) noexcept: CrcImpl(std::move(impl)) {}
};

}
