#pragma once

#include "ch.h"
#include "hal_streams.h"
#include "hal_channels.h"
#include "osal.h"

#include <SEGGER_RTT.h>

#include <limits>

constexpr systime_t systimeDiff(systime_t currentTime, systime_t prevTime) {
    return currentTime >= prevTime
        ? currentTime - prevTime
        : std::numeric_limits<systime_t>::max() - (prevTime - currentTime);
}

template <unsigned BUFFER_INDEX, uint32_t READ_POLL_PERIOD_MS>
class RTTStreamT: public BaseSequentialStream, public BaseChannel {
public:
    RTTStreamT()
        : BaseSequentialStream{(BaseSequentialStreamVMT*)&vmt_}
        , BaseChannel{&vmt_}
    {
    }

private:
    static RTTStreamT* cast(void* p) { return static_cast<RTTStreamT*>(p); }
    
private:
    static size_t write(void *ip, const uint8_t *bp, size_t n) { return cast(ip)->write(bp, n); }
    static size_t read(void *ip, uint8_t *bp, size_t n) { return cast(ip)->read(bp, n); }
    static msg_t put(void *ip, uint8_t b) { return cast(ip)->put(b); }
    static msg_t get(void *ip) { return cast(ip)->get(); }
    static size_t writet(
        void *ip,
        const uint8_t *bp,
        size_t n,
        systime_t time) {
        return cast(ip)->writet(bp, n, time);
    }
    static size_t readt(void *ip, uint8_t *bp, size_t n, systime_t time) {
        return cast(ip)->readt(bp, n, time);
    }
    static msg_t putt(void *ip, uint8_t b, systime_t time) { return cast(ip)->putt(b, time); }
    static msg_t gett(void *ip, systime_t time) { return cast(ip)->gett(time); }

private:
    size_t writet(const uint8_t *bp, size_t n, systime_t) {
        return SEGGER_RTT_Write(BUFFER_INDEX, bp, n);
    }

    size_t readt(uint8_t *bp, size_t n, systime_t timeout) {
        systime_t prevTime = osalOsGetSystemTimeX();
        size_t read = 0;
        while(true) {
            read = SEGGER_RTT_Read(BUFFER_INDEX, bp, n);
            if (read) {
                return read;
            }
            chThdSleepMilliseconds(READ_POLL_PERIOD_MS);
            if (timeout != TIME_INFINITE) {
                systime_t currentTime = osalOsGetSystemTimeX();
                systime_t diff = systimeDiff(currentTime, prevTime);
                if (diff > timeout) {
                    return read;
                }
                timeout -= diff;
                prevTime = currentTime;
            }
        }
    }

    msg_t putt(uint8_t b, systime_t time) {
        switch (writet(&b, 1, time)) {
            case 1: return MSG_OK;
            default: return MSG_RESET;
        }
    }
    
    msg_t gett(systime_t time) {
        uint8_t b;
        switch (readt(&b, 1, time)) {
            case 1: return b;
            default: return MSG_RESET;
        }
    }

    msg_t put(uint8_t b) {
        return putt(b, TIME_INFINITE);
    }

    msg_t get() {
        return gett(TIME_INFINITE);
    }
    
    size_t write(const uint8_t *bp, size_t n) {
        return writet(bp, n, TIME_INFINITE);
    }

    size_t read(uint8_t *bp, size_t n) {
        return readt(bp, n, TIME_INFINITE);
    }
    
private:
    static const BaseChannelVMT vmt_;
};

template <unsigned BUFFER_INDEX, uint32_t READ_POLL_PERIOD_MS>
const BaseChannelVMT RTTStreamT<BUFFER_INDEX, READ_POLL_PERIOD_MS>::vmt_ = {
    RTTStreamT::write,
    RTTStreamT::read,
    RTTStreamT::put,
    RTTStreamT::get,
    RTTStreamT::putt,
    RTTStreamT::gett,
    RTTStreamT::writet,
    RTTStreamT::readt
};
