#pragma once

#include <cstdint>

class Packetizer2 {
public:
    enum Action { Skip, Store, Finished };

private:
    using State = Action (Packetizer2::*)(uint8_t byte);

public:
    Packetizer2()
        : state(&Packetizer2::consumeSync)
    {}

    Action consume(std::uint8_t byte) {
        return (this->*state)(byte);
    }

private:
    Action consumeSync(std::uint8_t byte) {
        if (byte == 0) {
            state = &Packetizer2::consumeSkipDelim;
        }
        return Skip;
    }
    
    Action consumeSkipDelim(std::uint8_t byte) {
        if (byte == 0) {
            return Skip;
        }
        state = &Packetizer2::consumeWithin;
        return Store;
    }

    Action consumeWithin(std::uint8_t byte) {
        if (byte == 0) {
            state = &Packetizer2::consumeSkipDelim;
            return Finished;
        }
        return Store;
    }

private:
    State state;
};
