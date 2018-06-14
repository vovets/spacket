#pragma once

#include "ch.h"
#include "stm32_rcc.h"

namespace stm32 {

class CrcImpl {
public:
    template <typename Crc>
    static Crc open(CRC_TypeDef* device) {
        return Crc(CrcImpl(device));
    }

    CrcImpl(CrcImpl&& src) noexcept: device(src.device) {
        src.device = nullptr;
    }

    CrcImpl& operator=(CrcImpl&& src) noexcept {
        device = src.device;
        src.device = nullptr;
        return *this;
    }

    ~CrcImpl() noexcept {
        if (device) {
            rccDisableAHB(RCC_AHBENR_CRCEN, FALSE);
            device = nullptr;
        }
    }

    uint32_t add(uint32_t v) {
        device->DR = v;
        return device->DR;
    }

    uint32_t add(const uint8_t* data, size_t size);

    uint32_t last() {
        return device->DR;
    }

private:
    CrcImpl(CRC_TypeDef* device)
        : device(device) {
        rccEnableAHB(RCC_AHBENR_CRCEN, FALSE);
        device->CR |= CRC_CR_RESET;
    }

    CRC_TypeDef* device;
};

}
