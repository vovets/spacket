#pragma once

#include <spacket/module.h>


template <typename Buffer>
struct AddressT {
    std::uint8_t channel;

    bool matches(const Buffer& buffer) {
        return buffer.size() != 0 && buffer.begin()[0] == channel;
    }

    bool operator==(const AddressT& other) {
        return channel == other.channel;
    }

    Result<Buffer> stripHeader(Buffer&& buffer) {
        return buffer.suffix(buffer.begin() + 1);
    }

    Result<Buffer> addHeader(Buffer&& buffer) {
        if (buffer.size() == Buffer::maxSize()) {
            return { toError(ErrorCode::AddressNoRoom) };
        }
        auto s = buffer.size();
        buffer.resize(s + 1);
        std::memmove(buffer.begin() + 1, buffer.begin(), s);
        buffer.begin()[0] = channel;
        return ok(std::move(buffer));
    }
};

template <typename Buffer, typename Address, std::size_t MAX_STACKS>
class MultistackT: public ModuleT<Buffer> {
    using This = MultistackT<Buffer, Address, MAX_STACKS>;
    using DeferredProc = DeferredProcT<Buffer>;
    using Module = ModuleT<Buffer>;
    using ModuleList = ModuleListT<Buffer>;

    using Module::ops;

    struct UpperStack: ModuleOpsT<Buffer>, ModuleT<Buffer>  {
        This* lowerStack;
        Address address;
        ModuleList moduleList;
        using Module::ops;

        UpperStack(This* lowerStack, const Address& address)
            : lowerStack(lowerStack)
            , address(address)
        {}

        void push(Module& module) {
            module.ops = this;
            moduleList.push_back(module);
        }
        
        Result<Module*> lower(Module& m) override {
            auto it = typename ModuleList::reverse_iterator(moduleList.iterator_to(m));
            if (it == moduleList.rend()) {
                return ok<Module*>(lowerStack);
            }
            return ok(&(*it));
        }
        
        Result<Module*> upper(Module& m) override {
            auto it = ++moduleList.iterator_to(m);
            if (it == moduleList.end()) {
                return { toError(ErrorCode::ModuleNoUpper) };
            }
            return ok(&(*it));
        }
        
        Result<Void> deferIO(DeferredProc&& dp) override {
            return lowerStack->ops->deferIO(std::move(dp));
        }

        Result<Void> deferProc(DeferredProc&& dp) override {
            return lowerStack->ops->deferProc(std::move(dp));
        }

        Result<Void> up(Buffer&& b) override {
            cpm::dpb("UpperStack::up|", &b);
            cpm::dpl("UpperStack::up|channel %d", address.channel);
            return
            address.stripHeader(std::move(b)) >=
            [&] (Buffer&& stripped) {
                return
                ops->upper(*this) >=
                [&] (Module* m) {
                    return ops->deferProc(makeProc(std::move(stripped), *m, &Module::up));
                };
            };
        }
        
        Result<Void> down(Buffer&& b) override {
            cpm::dpb("UpperStack::down|", &b);
            cpm::dpl("UpperStack::down|channel %d", address.channel);
            return
            address.addHeader(std::move(b)) >=
            [&] (Buffer&& withHeader) {
                return
                ops->lower(*this) >=
                [&] (Module* m) {
                    return ops->deferProc(makeProc(std::move(withHeader), *m, &Module::down));
                };
            };
        }
    };
    
    using UpperStacksStorage = std::array<Storage<UpperStack>, MAX_STACKS>;
    UpperStacksStorage upperStacksStorage;
    std::size_t firstFree = 0;

    UpperStack& at(std::size_t index) {
        return *reinterpret_cast<UpperStack*>(&upperStacksStorage[index]);
    }

    UpperStack& findOrCreate(const Address& address) {
        if (firstFree >= upperStacksStorage.size()) {
            FATAL_ERROR("MultistackT::findOrCreate");
        }
        for (std::size_t i = 0; i < firstFree; ++i) {
            UpperStack& r = at(i);
            if (r.address == address) { return r; }
        }
        UpperStack* us = ::new (&upperStacksStorage[firstFree++]) UpperStack(this, address);
        // UpperStack IS a Module and must be the lowest module in the stack
        us->push(*us);
        return *us;
    }

public:
    void push(const Address& address, Module& module) {
        UpperStack& us = findOrCreate(address);
        us.push(module);
    }
    
    Result<Void> up(Buffer&& b) override {
        cpm::dpb("MultistackT::up|", &b);
        for (std::size_t i = 0; i < firstFree; ++i) {
            UpperStack& us = at(i);
            if (us.address.matches(b)) {
                cpm::dpl("MultistackT::up|matched channel %d", us.address.channel);
                return ops->deferProc(makeProc(std::move(b), us, &Module::up));
            }
        }
        cpm::dpl("MultistackT::up|not matched");
        return
        ops->upper(*this) >=
        [&] (Module* m) {
            return ops->deferProc(makeProc(std::move(b), *m, &Module::up));
        };
    }

    Result<Void> down(Buffer&& b) override {
        cpm::dpb("MultistackT::down|", &b);
        return
        ops->lower(*this) >=
        [&] (Module* m) {
            return ops->deferProc(makeProc(std::move(b), *m, &Module::down));
        };
    }
};
