#pragma once

#include "ch.h"
#include "stm32_rcc.h"

#include <cstdint>


namespace stm32 {

class CrcDevice {
public:
    static CrcDevice open(CRC_TypeDef* device) {
        return CrcDevice(device);
    }

    CrcDevice(CrcDevice&& src) noexcept: device(src.device) {
        src.device = nullptr;
    }

    CrcDevice& operator=(CrcDevice&& src) noexcept {
        device = src.device;
        src.device = nullptr;
        return *this;
    }

    ~CrcDevice() noexcept {
        if (device) {
            rccDisableAHB(RCC_AHBENR_CRCEN, FALSE);
            device = nullptr;
        }
    }

    void reset() {
        device->CR |= CRC_CR_RESET;
    }
    
    std::uint32_t add(std::uint32_t v) {
        device->DR = v;
        return device->DR;
    }

    std::uint32_t last() {
        return device->DR;
    }

private:
    CrcDevice(CRC_TypeDef* device)
        : device(device) {
        rccEnableAHB(RCC_AHBENR_CRCEN, FALSE);
        reset();
    }

    CRC_TypeDef* device;
};

std::uint32_t crc(CrcDevice& device, const std::uint8_t* buffer, std::size_t size);

} // stm32
