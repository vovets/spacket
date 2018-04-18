#pragma once

#include "ch.h"

//#include <spacket/guard.h>

Error toError(msg_t msg) {
    switch (msg) {
        case MSG_TIMEOUT: return Error::ChMsgTimeout;
        case MSG_RESET:   return Error::ChMsgReset;
    }
    return Error::MiserableFailure1;
}

template <typename Message, size_t SIZE>
class MailboxT {
public:
    MailboxT();

    Result<boost::blank> post(Message& msg, Timeout timeout);
    Result<boost::blank> postS(Message& msg, Timeout timeout);
        
    Result<Message> fetch(Timeout timeout);
    Result<Message> fetchS(Timeout timeout);

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
MailboxT<Message, SIZE>::MailboxT()
    : wrptr(buffer())
    , rdptr(buffer())
    , cnt(0)
    , reset(false)
    , qw(_THREADS_QUEUE_DATA(qw))
    , qr(_THREADS_QUEUE_DATA(qr))
{
}

template <typename Message, size_t SIZE>
Result<boost::blank> MailboxT<Message, SIZE>::post(Message& msg, Timeout timeout) {
    // auto g = guard(chSysLock, chSysUnlock);
    chSysLock();
    auto result = postS(msg, timeout);
    chSysUnlock();
    return std::move(result);
}

template <typename Message, size_t SIZE>
Result<boost::blank> MailboxT<Message, SIZE>::postS(Message& msg, Timeout timeout) {

    chDbgCheckClassS();

    msg_t rdymsg;
    do {
        /* If the mailbox is in reset state then returns immediately.*/
        if (reset) {
            return fail<boost::blank>(Error::ChMsgReset);
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

            return ok(boost::blank{});
        }

        /* No space in the queue, waiting for a slot to become available.*/
        rdymsg = chThdEnqueueTimeoutS(&qw, toSystime(timeout));
    } while (rdymsg == MSG_OK);

    return fail<boost::blank>(toError(rdymsg));
}

template <typename Message, size_t SIZE>
Result<Message> MailboxT<Message, SIZE>::fetch(Timeout timeout) {
    chSysLock();
    auto result = fetchS(timeout);
    chSysUnlock();
    return std::move(result);
}

template <typename Message, size_t SIZE>
Result<Message> MailboxT<Message, SIZE>::fetchS(Timeout timeout) {

    chDbgCheckClassS();

    msg_t rdymsg;
    do {
        /* If the mailbox is in reset state then returns immediately.*/
        if (reset) {
            return fail<Message>(Error::ChMsgReset);
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
size_t MailboxT<Message, SIZE>::usedCountI() {
    chDbgCheckClassI();
    return cnt;
}

template <typename Message, size_t SIZE>
size_t MailboxT<Message, SIZE>::freeCountI() {
    chDbgCheckClassI();
    return SIZE - usedCountI();
}
