#pragma once

#include <spacket/result.h>

#include <functional>

template <typename Buffer>
struct PacketDecodeFSMT {
    using This = PacketDecodeFSMT<Buffer>;
    using PacketFinishedCallback = std::function<Result<boost::blank>(Buffer&)>;
    using State = Result<boost::blank> (This::*)(std::uint8_t byte);

    PacketDecodeFSMT(
        PacketFinishedCallback packetFinishedCallback,
        Buffer&& buffer)
        : packetFinishedCallback(packetFinishedCallback)
        , buffer(std::move(buffer))
        , state(&This::delimiter)
        , blockBytesLeft(0)
        , bytesRead(0)
        , blockAppendZero(false)
    {}

    Result<boost::blank> consume(std::uint8_t byte) {
        return (this->*state)(byte);
    }
    
    void transit(State toState) {
        state = toState;
    }

    Result<boost::blank> delimiter(std::uint8_t byte);
    Result<boost::blank> start(std::uint8_t byte);
    Result<boost::blank> block(std::uint8_t byte);
    Result<boost::blank> nextBlock(std::uint8_t byte);

    void startBlock(std::uint8_t code);
    Result<boost::blank> finishBlock();
    Result<boost::blank> append(std::uint8_t byte);
    Result<boost::blank> finishPacket();

    PacketFinishedCallback packetFinishedCallback;
    Buffer buffer;
    State state;
    std::size_t blockBytesLeft;
    std::size_t bytesRead;
    bool blockAppendZero;
};

template <typename Buffer>
Result<boost::blank> PacketDecodeFSMT<Buffer>::delimiter(std::uint8_t byte) {
    switch (byte) {
        case 0:
            transit(&This::start);
        default:
            ;
    }
    return ok(boost::blank());
}

template <typename Buffer>
Result<boost::blank> PacketDecodeFSMT<Buffer>::start(std::uint8_t byte) {
    switch (byte) {
        case 0:
            break;
        case 1:
            startBlock(byte);
            return
            finishBlock() >
            [&] { transit(&This::nextBlock); return ok(boost::blank()); } <=
            [&] (Error e) { transit(&This::delimiter); return fail<boost::blank>(e); };
        default:
            startBlock(byte);
            transit(&This::block);
            break;
    }
    return ok(boost::blank());
}
            
template <typename Buffer>
Result<boost::blank> PacketDecodeFSMT<Buffer>::block(std::uint8_t byte) {
    switch (byte) {
        case 0:
            transit(&This::delimiter);
            return fail<boost::blank>(toError(ErrorCode::CobsBadEncoding));
        default:
            return
            append(byte) >
            [&] {
                if (blockBytesLeft == 0) {
                    return
                    finishBlock() >
                    [&] { transit(&This::nextBlock); return ok(boost::blank()); };
                }
                return ok(boost::blank());
            } <=
            [&] (Error e) { transit(&This::delimiter); return fail<boost::blank>(e); };
    }
    return ok(boost::blank());
}

template <typename Buffer>
Result<boost::blank> PacketDecodeFSMT<Buffer>::nextBlock(std::uint8_t byte) {
    switch (byte) {
        case 0:
            return
            finishPacket() >
            [&] { transit(&This::start); return ok(boost::blank()); } <=
            [&] (Error e) { transit(&This::delimiter); return fail<boost::blank>(e); };
        case 1:
            startBlock(byte);
            return finishBlock();
        default:
            startBlock(byte);
            transit(&This::block);
            break;
    }
    return ok(boost::blank());
}

template <typename Buffer>
void PacketDecodeFSMT<Buffer>::startBlock(std::uint8_t code) {
    blockBytesLeft = code - 1;
    blockAppendZero = code != 255;
}

template <typename Buffer>
Result<boost::blank> PacketDecodeFSMT<Buffer>::finishBlock() {
    if (blockAppendZero) {
        return append(0);
    }
    return ok(boost::blank());
}

template <typename Buffer>
Result<boost::blank> PacketDecodeFSMT<Buffer>::append(std::uint8_t byte) {
    if (bytesRead >= buffer.maxSize()) {
        return fail<boost::blank>(toError(ErrorCode::PacketTooBig));
    }
    *(buffer.begin() + bytesRead++) = byte;
    --blockBytesLeft;
    return ok(boost::blank());
}

template <typename Buffer>
Result<boost::blank> PacketDecodeFSMT<Buffer>::finishPacket() {
    if (buffer.begin()[bytesRead - 1] == 0) { --bytesRead; }
    if (bytesRead == 0) {
        return fail<boost::blank>(toError(ErrorCode::CobsBadEncoding));
    }
    buffer.resize(bytesRead);
    bytesRead = 0;
    return packetFinishedCallback(buffer);
}
