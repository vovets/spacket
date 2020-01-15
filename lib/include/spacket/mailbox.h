#pragma once

#include <spacket/impl/mailbox_impl_p.h>

template <typename Message>
struct MailboxT: MailboxImplT<Message> {
    using Impl = MailboxImplT<Message>;
    
    Result<Void> replace(Message& message) {
        return Impl::replace(message);
    }
    
    Result<Void> post(Message& message, Timeout timeout) {
        return Impl::post(message, timeout);
    }
    
    Result<Message> fetch(Timeout timeout) {
        return Impl::fetch(timeout);
    }
};
