#include <Fw/Types/Assert.hpp>
#include <Os/Queue.hpp>
#include <Os/Task.hpp>
#include <FreeRTOS.h>
#include <os_queue.h>

namespace Os {

    Queue::Queue() :
        m_handle(0) {
    }

    Queue::QueueStatus Queue::create(const Fw::StringBase &name, NATIVE_INT_TYPE depth, NATIVE_INT_TYPE msgSize) {

        QueueHandle_t queueHandle = xQueueCreate( depth, msgSize );

        if (NULL == queueHandle) {
          return QUEUE_UNINITIALIZED;
        }

        this->m_handle = (POINTER_CAST) queueHandle;
        
        Queue::s_numQueues++;

        return QUEUE_OK;
    }

    Queue::~Queue() {
    }

    Queue::QueueStatus Queue::send(const U8* buffer, NATIVE_INT_TYPE size, NATIVE_INT_TYPE priority, QueueBlocking block) {

        if (NULL == buffer) {
            return QUEUE_EMPTY_BUFFER;
        }

        QueueHandle_t* queueHandle = (QueueHandle_t*) this->m_handle;

        BaseType_t stat;

        if (Os::Task::isISR()) {
            stat = xQueueSendFromISR( queueHandle, buffer, 0 ); // don't block in ISR
        } else {
            if (QUEUE_BLOCKING == block) {
                stat = xQueueSend( queueHandle, buffer, 10 );
            } else {
                stat = xQueueSend( queueHandle, buffer, 0 );
            }
        }

        if (pdTRUE == stat) {
            return QUEUE_OK;
        } else {
            return QUEUE_FULL;
        }

    }

    Queue::QueueStatus Queue::receive(U8* buffer, NATIVE_INT_TYPE capacity, NATIVE_INT_TYPE &actualSize, NATIVE_INT_TYPE &priority, QueueBlocking block) {
#if 0

        QueueHandle* queueHandle = (QueueHandle*) this->m_handle;
        mqd_t handle = queueHandle->handle;
        pthread_cond_t* queueNotEmpty = &queueHandle->queueNotEmpty;
        pthread_cond_t* queueNotFull = &queueHandle->queueNotFull;
        pthread_mutex_t* mp = &queueHandle->mp;

        if (-1 == handle) {
            return QUEUE_UNINITIALIZED;
        }

        ssize_t size;
        int ret; 
        bool notFinished = true;
        while (notFinished) {
            size = mq_receive(handle, (char*) buffer, (size_t) capacity,
#ifdef TGT_OS_TYPE_VXWORKS        		
                        (int*)&priority);
#else
                        (unsigned int*) &priority);
#endif        

            if (-1 == size) { // error
                switch (errno) {
                    case EINTR:
                        continue;
                    case EMSGSIZE:
                        return QUEUE_SIZE_MISMATCH;
                    case EAGAIN:
                        if (block == QUEUE_NONBLOCKING) {
                            // no more messages. If we are
                            // non-blocking, return
                            return QUEUE_NO_MORE_MSGS;
                        } else {
                            // Go to sleep until we receive a signal that something was put on the queue:
                            // Note: pthread_cont_wait must be called "with mutex locked by the calling 
                            // thread or undefined behavior results." - from the docs
                            ret = pthread_mutex_lock(mp);
                            FW_ASSERT(ret == 0, errno);
                            ret = pthread_cond_wait(queueNotEmpty, mp);
                            FW_ASSERT(ret == 0, ret); // If this fails, something horrible happened.
                            ret = pthread_mutex_unlock(mp);
                            FW_ASSERT(ret == 0, errno);
                            continue;
                        }
                        break;
                    default:
                        return QUEUE_UNKNOWN_ERROR;
                }
            }
            else {
                notFinished = false;
                // Wake up a thread that might be waiting on the other end of the queue:
                ret = pthread_cond_signal(queueNotFull);
                FW_ASSERT(ret == 0, ret); // If this fails, something horrible happened.
            }
        }

        actualSize = (NATIVE_INT_TYPE) size;
#endif
        return QUEUE_OK;
    }

    NATIVE_INT_TYPE Queue::getNumMsgs(void) const {
#if 0
        QueueHandle* queueHandle = (QueueHandle*) this->m_handle;
        mqd_t handle = queueHandle->handle;

        struct mq_attr attr;
        int status = mq_getattr(handle, &attr);
        FW_ASSERT(status == 0);
        return (U32) attr.mq_curmsgs;
#endif
        return 0;
    }

    NATIVE_INT_TYPE Queue::getMaxMsgs(void) const {
        //FW_ASSERT(0);
        return 0;
    }

    NATIVE_INT_TYPE Queue::getQueueSize(void) const {
#if 0
        QueueHandle* queueHandle = (QueueHandle*) this->m_handle;
        mqd_t handle = queueHandle->handle;

        struct mq_attr attr;
        int status = mq_getattr(handle, &attr);
        FW_ASSERT(status == 0);
        return (U32) attr.mq_maxmsg;
#endif
        return 0;
    }

    NATIVE_INT_TYPE Queue::getMsgSize(void) const {
#if 0
        QueueHandle* queueHandle = (QueueHandle*) this->m_handle;
        mqd_t handle = queueHandle->handle;

        struct mq_attr attr;
        int status = mq_getattr(handle, &attr);
        FW_ASSERT(status == 0);
        return (U32) attr.mq_msgsize;
#endif
        return 0;
    }

}

