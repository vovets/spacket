#pragma once

#include <spacket/impl/mailbox_impl_p.h>

template <typename Message>
struct MailboxT: MailboxImplT<Message> {
    using Impl = MailboxImplT<Message>;
    
    Result<boost::blank> replace(Message& message) {
        return Impl::replace(message);
    }
    
    Result<boost::blank> post(Message& message, Timeout timeout) {
        return Impl::post(message, timeout);
    }
    
    Result<Message> fetch(Timeout timeout) {
        return Impl::fetch(timeout);
    }
};
