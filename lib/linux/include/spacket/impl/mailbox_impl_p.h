#pragma once

#include <boost/optional.hpp>

#include <condition_variable>
#include <mutex>


template <typename Message>
struct MailboxImplT {
    Result<Void> replace(Message& message_) {
        std::lock_guard<std::mutex> lock(mutex);
        message = std::move(message_);
        full.notify_one();
        return ok();
    }
    
    Result<Message> fetch(Timeout timeout) {
        std::unique_lock<std::mutex> lock(mutex);
        if (full.wait_for(lock, timeout, [this] { return !!message; })) {
            Message m = std::move(*message);
            message = boost::optional<Message>();
            return ok(std::move(m));
        }
        return fail<Message>(toError(ErrorCode::Timeout));
    }

    boost::optional<Message> message;
    std::mutex mutex;
    std::condition_variable full;
};
