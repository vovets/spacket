#pragma once

#include <spacket/result.h>

#include <functional>


struct PacketDecodeFSM {
    using This = PacketDecodeFSM;
    using PacketFinishedCallback = std::function<Result<Void>(Buffer&)>;
    using State = Result<Void> (This::*)(std::uint8_t byte);

    PacketDecodeFSM(
        PacketFinishedCallback packetFinishedCallback,
        Buffer&& buffer)
        : packetFinishedCallback(packetFinishedCallback)
        , buffer(std::move(buffer))
        , state(&This::delimiter)
        , blockBytesLeft(0)
        , bytesRead(0)
        , blockAppendZero(false)
    {}

    Result<Void> consume(std::uint8_t byte) {
        return (this->*state)(byte);
    }
    
    void transit(State toState) {
        state = toState;
    }

    Result<Void> delimiter(std::uint8_t byte);
    Result<Void> start(std::uint8_t byte);
    Result<Void> block(std::uint8_t byte);
    Result<Void> nextBlock(std::uint8_t byte);

    void startBlock(std::uint8_t code);
    Result<Void> finishBlock();
    Result<Void> append(std::uint8_t byte);
    Result<Void> finishPacket();

    PacketFinishedCallback packetFinishedCallback;
    Buffer buffer;
    State state;
    std::size_t blockBytesLeft;
    std::size_t bytesRead;
    bool blockAppendZero;
};

Result<Void> PacketDecodeFSM::delimiter(std::uint8_t byte) {
    switch (byte) {
        case 0:
            transit(&This::start);
        default:
            ;
    }
    return ok();
}

Result<Void> PacketDecodeFSM::start(std::uint8_t byte) {
    switch (byte) {
        case 0:
            break;
        case 1:
            startBlock(byte);
            return
            finishBlock() >
            [&] { transit(&This::nextBlock); return ok(); } <=
            [&] (Error e) { transit(&This::delimiter); return fail(e); };
        default:
            startBlock(byte);
            transit(&This::block);
            break;
    }
    return ok();
}
            
Result<Void> PacketDecodeFSM::block(std::uint8_t byte) {
    switch (byte) {
        case 0:
            transit(&This::delimiter);
            return fail(toError(ErrorCode::CobsBadEncoding));
        default:
            return
            append(byte) >
            [&] {
                if (blockBytesLeft == 0) {
                    return
                    finishBlock() >
                    [&] { transit(&This::nextBlock); return ok(); };
                }
                return ok();
            } <=
            [&] (Error e) { transit(&This::delimiter); return fail(e); };
    }
    return ok();
}

Result<Void> PacketDecodeFSM::nextBlock(std::uint8_t byte) {
    switch (byte) {
        case 0:
            return
            finishPacket() >
            [&] { transit(&This::start); return ok(); } <=
            [&] (Error e) { transit(&This::delimiter); return fail(e); };
        case 1:
            startBlock(byte);
            return finishBlock();
        default:
            startBlock(byte);
            transit(&This::block);
            break;
    }
    return ok();
}

void PacketDecodeFSM::startBlock(std::uint8_t code) {
    blockBytesLeft = code - 1;
    blockAppendZero = code != 255;
}

Result<Void> PacketDecodeFSM::finishBlock() {
    if (blockAppendZero) {
        return append(0);
    }
    return ok();
}

Result<Void> PacketDecodeFSM::append(std::uint8_t byte) {
    if (bytesRead >= buffer.maxSize()) {
        return fail(toError(ErrorCode::PacketTooBig));
    }
    *(buffer.begin() + bytesRead++) = byte;
    --blockBytesLeft;
    return ok();
}

Result<Void> PacketDecodeFSM::finishPacket() {
    if (buffer.begin()[bytesRead - 1] == 0) { --bytesRead; }
    if (bytesRead == 0) {
        return fail(toError(ErrorCode::CobsBadEncoding));
    }
    buffer.resize(bytesRead);
    bytesRead = 0;
    return packetFinishedCallback(buffer);
}
