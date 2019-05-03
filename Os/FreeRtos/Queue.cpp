#include <Fw/Types/Assert.hpp>
#include <Os/Queue.hpp>
#include <Os/Task.hpp>
#include <FreeRTOS.h>
#include <os_queue.h>

namespace Os {

    struct qInfo {
        QueueHandle_t handle;
        NATIVE_UINT_TYPE depth;
        NATIVE_UINT_TYPE msgSize;
    };

    Queue::Queue() :
        m_handle(0) {
    }

    Queue::QueueStatus Queue::create(const Fw::StringBase &name, NATIVE_INT_TYPE depth, NATIVE_INT_TYPE msgSize) {

        QueueHandle_t queueHandle = xQueueCreate( depth, msgSize );

        if (NULL == queueHandle) {
          return QUEUE_UNINITIALIZED;
        }

        qInfo *q_ptr = new qInfo;
        q_ptr->handle = queueHandle;
        q_ptr->depth = depth;
        q_ptr->msgSize = msgSize;

        this->m_handle = (POINTER_CAST) q_ptr;
        
        Queue::s_numQueues++;

        return QUEUE_OK;
    }

    Queue::~Queue() {
        qInfo *q_ptr = (qInfo*) this->m_handle;
        if (q_ptr) {
            vQueueDelete(q_ptr->handle);
            delete q_ptr;
        }
    }

    Queue::QueueStatus Queue::send(const U8* buffer, NATIVE_INT_TYPE size, NATIVE_INT_TYPE priority, QueueBlocking block) {

        if (NULL == buffer) {
            return QUEUE_EMPTY_BUFFER;
        }

        if (NULL == this->m_handle) {
            return QUEUE_UNINITIALIZED;
        }

        qInfo *q_ptr = (qInfo*) this->m_handle;

        BaseType_t stat = errQUEUE_FULL;

        if (Os::Task::isISR()) {
            stat = xQueueSendFromISR( q_ptr->handle, buffer, 0 ); // don't block in ISR
        } else {
            if (QUEUE_BLOCKING == block) {
                while (errQUEUE_FULL == stat) {
                    stat = xQueueSend( q_ptr->handle, buffer, 10 );
                }
            } else {
                stat = xQueueSend( q_ptr->handle, buffer, 0 );
            }
        }

        if (pdTRUE == stat) {
            return QUEUE_OK;
        } else {
            return QUEUE_FULL;
        }

    }

    Queue::QueueStatus Queue::receive(U8* buffer, NATIVE_INT_TYPE capacity, NATIVE_INT_TYPE &actualSize, NATIVE_INT_TYPE &priority, QueueBlocking block) {

        if (NULL == buffer) {
            return QUEUE_EMPTY_BUFFER;
        }

        if (NULL == this->m_handle) {
            return QUEUE_UNINITIALIZED;
        }

        qInfo *q_ptr = (qInfo*) this->m_handle;

        BaseType_t stat = errQUEUE_FULL;

        if (QUEUE_BLOCKING == block) {
            do {
                stat = xQueueReceive(q_ptr->handle, (void*)buffer, portMAX_DELAY );
            } while (errQUEUE_FULL == stat);
        } else {
            stat = xQueueReceive(q_ptr->handle, (void*)buffer, 0 );
        }

        if (pdTRUE == stat) {
            return QUEUE_OK;
        } else {
            return QUEUE_NO_MORE_MSGS;
        }

    }

    NATIVE_INT_TYPE Queue::getNumMsgs(void) const {

        if (NULL == this->m_handle) {
            return 0;
        }

        qInfo *q_ptr = (qInfo*) this->m_handle;
        return uxQueueMessagesWaiting(q_ptr->handle);

    }

    NATIVE_INT_TYPE Queue::getMaxMsgs(void) const {
        // FreeRtos doesn't seem to support high watermark
        return 0;
    }

    NATIVE_INT_TYPE Queue::getQueueSize(void) const {

        if (NULL == this->m_handle) {
            return 0;
        }

        qInfo *q_ptr = (qInfo*) this->m_handle;
        return q_ptr->depth;
    }

    NATIVE_INT_TYPE Queue::getMsgSize(void) const {

        if (NULL == this->m_handle) {
            return 0;
        }

        qInfo *q_ptr = (qInfo*) this->m_handle;
        return q_ptr->msgSize;
    }

}

