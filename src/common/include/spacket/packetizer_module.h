#pragma once

#include <spacket/module.h>
#include <spacket/packetizer2.h>
#include <spacket/buffer_box.h>


struct PacketizerModule: Module {
    alloc::Allocator& allocator;
    Packetizer2 packetizer;
    BufferBox buffer;
    std::uint8_t* writePtr = nullptr;

    PacketizerModule(alloc::Allocator& allocator): allocator(allocator) {}

    Result<Void> up(Buffer&& b) override {
        cpm::dpb("PacketizerModule::up|", &b);
        for (auto readPtr = b.begin(); readPtr != b.end(); ++readPtr) {
            auto action = packetizer.consume(*readPtr);
            switch (action) {
                case Packetizer2::Skip:
                    continue;
                        
                case Packetizer2::Store:
                    if (!buffer) {
                        auto result =
                        Buffer::create(allocator) >=
                        [&](Buffer&& c) {
                            buffer = std::move(c);
                            writePtr = buffer->begin();
                            return ok();
                        };
                            
                        if (isFail(result)) {
                            cpm::dpl("PacketizerModule::up|buffer allocation failed");
                            packetizer = Packetizer2();
                            return result;
                        }
                    }

                    if (writePtr == buffer->end()) {
                        return fail(toError(ErrorCode::PacketizerOverflow));
                    }
                        
                    *writePtr++ = *readPtr;
                    continue;
                        
                case Packetizer2::Finished:
                    if (!buffer) { FATAL_ERROR("PacketizerModule::up"); }
                    buffer->resize(writePtr - buffer->begin());
                    return deferUp(extract(buffer));
            }
        }
        return ok();
    }
        
    Result<Void> down(Buffer&& b) override {
        cpm::dpb("PacketizerModule::down|", &b);
        if (b.size() > (Buffer::maxSize(allocator) - 2)) {
            return fail(toError(ErrorCode::PacketizerOverflow));
        }
        return
        Buffer::create(allocator) >=
        [&](Buffer&& c) {
            c.resize(b.size() + 2);
            *c.begin() = 0;
            *(c.begin() + c.size() - 1) = 0;
            std::memcpy(c.begin() + 1, b.begin(), b.size());
            return deferDown(std::move(c));
        };
    }
};
