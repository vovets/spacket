#pragma once

#include "ch.h"

#include <spacket/time_utils.h>
#include <spacket/result.h>
#include <spacket/result_utils_base.h>

inline
Error toError(msg_t msg) {
    switch (msg) {
        case MSG_TIMEOUT: return toError(ErrorCode::Timeout);
        case MSG_RESET:   return toError(ErrorCode::ChMsgReset);
    }
    FATAL_ERROR("bad msg");
    // should not reach here
    return toError(ErrorCode::Timeout);
}

template <typename Message_, size_t SIZE_>
class QueueT {
public:
    using Message = Message_;
    static constexpr std::size_t SIZE = SIZE_;
    
public:
    QueueT();

    Result<Void> post(Message& msg, Timeout timeout);
    Result<Void> postS(Message& msg, Timeout timeout);
    Result<Void> postI(Message& msg);
        
    Result<Message> fetch(Timeout timeout);
    Result<Message> fetchS(Timeout timeout);
    Result<Message> fetchI();

private:
    size_t usedCountI();
    size_t freeCountI();
    Message* buffer() { return reinterpret_cast<Message*>(storage); }
    

private:
    uint8_t storage[SIZE * sizeof(Message)];
    Message* wrptr;
    Message* rdptr;
    size_t cnt;
    bool reset;
    threads_queue_t qw;
    threads_queue_t qr;
};

template <typename Message, size_t SIZE>
QueueT<Message, SIZE>::QueueT()
    : wrptr(buffer())
    , rdptr(buffer())
    , cnt(0)
    , reset(false)
    , qw(_THREADS_QUEUE_DATA(qw))
    , qr(_THREADS_QUEUE_DATA(qr))
{
}

template <typename Message, size_t SIZE>
Result<Void> QueueT<Message, SIZE>::post(Message& msg, Timeout timeout) {
    chSysLock();
    auto result = postS(msg, timeout);
    chSysUnlock();
    return result;
}

template <typename Message, size_t SIZE>
Result<Void> QueueT<Message, SIZE>::postS(Message& msg, Timeout timeout) {

    chDbgCheckClassS();

    msg_t rdymsg;
    do {
        /* If the mailbox is in reset state then returns immediately.*/
        if (reset) {
            return fail(toError(ErrorCode::ChMsgReset));
        }

        /* Is there a free message slot in queue? if so then post.*/
        if (freeCountI() > 0) {
            new (wrptr) Message(std::move(msg));
            ++wrptr;
            if (wrptr >= buffer() + SIZE) {
                wrptr = buffer();
            }
            ++cnt;

            /* If there is a reader waiting then makes it ready.*/
            chThdDequeueNextI(&qr, MSG_OK);
            chSchRescheduleS();

            return ok();
        }

        /* No space in the queue, waiting for a slot to become available.*/
        rdymsg = chThdEnqueueTimeoutS(&qw, toSystime(timeout));
    } while (rdymsg == MSG_OK);

    return fail(toError(rdymsg));
}

template <typename Message, size_t SIZE>
Result<Void> QueueT<Message, SIZE>::postI(Message& msg) {

    chDbgCheckClassI();

    /* If the mailbox is in reset state then returns immediately.*/
    if (reset) {
        return fail(toError(ErrorCode::ChMsgReset));
    }

    /* Is there a free message slot in queue? if so then post.*/
    if (freeCountI() > 0) {
        new (wrptr) Message(std::move(msg));
        ++wrptr;
        if (wrptr >= buffer() + SIZE) {
            wrptr = buffer();
        }
        ++cnt;

        /* If there is a reader waiting then makes it ready.*/
        chThdDequeueNextI(&qr, MSG_OK);

        return ok();
    }

    /* No space in the queue, immediate timeout.*/
    return fail(toError(ErrorCode::ChMsgTimeout));
}

template <typename Message, size_t SIZE>
Result<Message> QueueT<Message, SIZE>::fetch(Timeout timeout) {
    chSysLock();
    auto result = fetchS(timeout);
    chSysUnlock();
    return result;
}

template <typename Message, size_t SIZE>
Result<Message> QueueT<Message, SIZE>::fetchS(Timeout timeout) {

    chDbgCheckClassS();

    msg_t rdymsg;
    do {
        /* If the mailbox is in reset state then returns immediately.*/
        if (reset) {
            return fail<Message>(toError(ErrorCode::ChMsgReset));
        }

        /* Is there a message in queue? if so then fetch.*/
        if (usedCountI() > 0) {
            auto tmp = Message(std::move(*rdptr));
            rdptr->~Message();

            ++rdptr;
            if (rdptr >= buffer() + SIZE) {
                rdptr = buffer();
            }
            --cnt;

            /* If there is a writer waiting then makes it ready.*/
            chThdDequeueNextI(&qw, MSG_OK);
            chSchRescheduleS();

            return ok(std::move(tmp));
        }

        /* No message in the queue, waiting for a message to become available.*/
        rdymsg = chThdEnqueueTimeoutS(&qr, toSystime(timeout));
    } while (rdymsg == MSG_OK);

    return fail<Message>(toError(rdymsg));
}

template <typename Message, size_t SIZE>
Result<Message> QueueT<Message, SIZE>::fetchI() {

    chDbgCheckClassI();

    /* If the mailbox is in reset state then returns immediately.*/
    if (reset) {
        return fail<Message>(toError(ErrorCode::ChMsgReset));
    }

    /* Is there a message in queue? if so then fetch.*/
    if (usedCountI() > 0) {
        auto tmp = Message(std::move(*rdptr));
        rdptr->~Message();

        ++rdptr;
        if (rdptr >= buffer() + SIZE) {
            rdptr = buffer();
        }
        --cnt;

        /* If there is a writer waiting then makes it ready.*/
        chThdDequeueNextI(&qw, MSG_OK);

        return ok(std::move(tmp));
    }

    /* No message in the queue, immediate timeout.*/
    return fail<Message>(toError(ErrorCode::ChMsgTimeout));
}

template <typename Message, size_t SIZE>
size_t QueueT<Message, SIZE>::usedCountI() {
    chDbgCheckClassI();
    return cnt;
}

template <typename Message, size_t SIZE>
size_t QueueT<Message, SIZE>::freeCountI() {
    chDbgCheckClassI();
    return SIZE - usedCountI();
}

template <typename Message>
struct MailboxImplT: QueueT<Message, 1> {
    using Base = QueueT<Message, 1>;
    
    Result<Void> replace(Message& message) {
        return
        Base::fetch(immediateTimeout()) >=
        [&] (Message) {
            return ok();
        } <=
        [&] (Error e) {
            if (e == toError(ErrorCode::Timeout)) {
                return ok();
            }
            return fail(e);
        } >
        [&] { return Base::post(message, immediateTimeout()); };
    }
};
