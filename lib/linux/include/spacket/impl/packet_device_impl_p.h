#pragma once

#include <spacket/result.h>
#include <spacket/serial_device.h>
#include <spacket/packet_device.h>
#include <spacket/impl/packet_device_impl_base.h>

#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/optional.hpp>

#include <thread>


template <typename Buffer>
struct PacketDeviceImpl: PacketDeviceImplBase<Buffer>,
    boost::intrusive_ref_counter<PacketDeviceImpl<Buffer>, boost::thread_unsafe_counter>
{
    using This = PacketDeviceImpl<Buffer>;
    using Base = PacketDeviceImplBase<Buffer>;
    using ThisPtr = boost::intrusive_ptr<This>;
    using SerialDevice = SerialDeviceT<Buffer>;
    using Base::dpl;

    static Result<ThisPtr> open(SerialDevice&& serialDevice);

    PacketDeviceImpl(Buffer&& buffer, SerialDevice&& serialDevice);
    virtual ~PacketDeviceImpl();

    Result<boost::blank> reportError(Error e) override;

    std::thread readThread;
};

template <typename Buffer>
Result<boost::intrusive_ptr<PacketDeviceImpl<Buffer>>> PacketDeviceImpl<Buffer>::open(SerialDevice&& serialDevice) {
    return
    Buffer::create(Buffer::maxSize()) >=
    [&] (Buffer&& buffer) {
        return ok(
            ThisPtr(
                new This(
                    std::move(buffer),
                    std::move(serialDevice))));
    };
}

template <typename Buffer>
PacketDeviceImpl<Buffer>::PacketDeviceImpl(
    Buffer&& buffer,
    SerialDevice&& serialDevice)
    : Base(
        std::move(buffer),
        std::move(serialDevice))
{
    readThread = std::thread([&] { this->readThreadFunction(); });
}

template <typename Buffer>
PacketDeviceImpl<Buffer>::~PacketDeviceImpl() {
    Base::requestStop();
    readThread.join();
}

template <typename Buffer>
 Result<boost::blank> PacketDeviceImpl<Buffer>::reportError(Error e) {
    return fail<boost::blank>(e);
}
