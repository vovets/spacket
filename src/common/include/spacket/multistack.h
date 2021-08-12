#pragma once

#include <spacket/module.h>


struct Address {
    std::uint8_t channel;

    bool matches(const Buffer& buffer) {
        return buffer.size() != 0 && buffer.begin()[0] == channel;
    }

    bool operator==(const Address& other) {
        return channel == other.channel;
    }

    Result<Buffer> stripHeader(Buffer&& buffer) {
        return buffer.suffix(buffer.begin() + 1);
    }

    Result<Buffer> addHeader(Buffer&& buffer) {
        if (buffer.size() == buffer.maxSize()) {
            return { toError(ErrorCode::AddressNoRoom) };
        }
        auto s = buffer.size();
        buffer.resize(s + 1);
        std::memmove(buffer.begin() + 1, buffer.begin(), s);
        buffer.begin()[0] = channel;
        return ok(std::move(buffer));
    }
};

template <typename Address, std::size_t MAX_STACKS>
class MultistackT: public Module {
    using Multistack = MultistackT<Address, MAX_STACKS>;

    struct Slot: Module {
        Multistack& multistack;
        Stack stack;
        Address address;

        Slot(Multistack& multistack, const Address& address)
            : multistack(multistack)
            , stack(multistack.executor(), multistack.procAllocator)
            , address(address)
        {
            stack.push(*this);
        }

        Result<Void> up(Buffer&& b) override {
            cpm::dpb("Slot::up|", &b);
            cpm::dpl("Slot::up|channel %d", address.channel);
            return
            address.stripHeader(std::move(b)) >=
            [&] (Buffer&& stripped) {
                return deferUp(std::move(stripped));
            };
        }
        
        Result<Void> down(Buffer&& b) override {
            cpm::dpb("Slot::down|", &b);
            cpm::dpl("Slot::down|channel %d", address.channel);
            return
            address.addHeader(std::move(b)) >=
            [&] (Buffer&& withHeader) {
                return multistack.down(std::move(withHeader));
            };
        }
    };
    
    using SlotsStorage = std::array<Storage<Slot>, MAX_STACKS>;
    
    SlotsStorage slotsStorage;
    std::size_t firstFree = 0;
    alloc::Allocator& procAllocator;

    Slot& at(std::size_t index) {
        return *reinterpret_cast<Slot*>(&slotsStorage[index]);
    }

    Slot& findOrCreate(const Address& address) {
        if (firstFree >= slotsStorage.size()) {
            FATAL_ERROR("MultistackT::findOrCreate");
        }
        for (std::size_t i = 0; i < firstFree; ++i) {
            Slot& r = at(i);
            if (r.address == address) { return r; }
        }
        Slot* slot = ::new (&slotsStorage[firstFree++]) Slot(*this, address);
        return *slot;
    }

public:
    MultistackT(alloc::Allocator& procAllocator): procAllocator(procAllocator) {}

    void push(const Address& address, Module& module) {
        Slot& slot = findOrCreate(address);
        slot.stack.push(module);
    }
    
    Result<Void> up(Buffer&& b) override {
        cpm::dpb("MultistackT::up|", &b);
        for (std::size_t i = 0; i < firstFree; ++i) {
            Slot& slot = at(i);
            if (slot.address.matches(b)) {
                cpm::dpl("MultistackT::up|matched channel %d", slot.address.channel);
                return slot.up(std::move(b));
            }
        }
        cpm::dpl("MultistackT::up|not matched");
        return deferUp(std::move(b));
    }

    Result<Void> down(Buffer&& b) override {
        cpm::dpb("MultistackT::down|", &b);
        return deferDown(std::move(b));
    }
};
