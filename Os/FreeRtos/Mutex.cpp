#include <Fw/Types/Assert.hpp>
#include <Os/Mutex.hpp>
#include <FreeRTOS.h>
#include <os_semphr.h>

namespace Os {
    
    Mutex::Mutex(void) {
        SemaphoreHandle_t xSemaphore = xSemaphoreCreateMutex();
        FW_ASSERT(xSemaphore);
        this->m_handle = (POINTER_CAST) xSemaphore;
    }
    
    Mutex::~Mutex(void) {
        if (this->m_handle) {
            vSemaphoreDelete((SemaphoreHandle_t)this->m_handle);
        }
    }
    
    void Mutex::lock(void) {
        FW_ASSERT(this->m_handle);
        // We want to wait forever
        BaseType_t stat = pdFALSE;
        do {
            BaseType_t stat = xSemaphoreTake((SemaphoreHandle_t)this->m_handle,portMAX_DELAY);
        } while (stat != pdTRUE);

    }

    void Mutex::unLock(void) {
        BaseType_t stat = xSemaphoreGive((SemaphoreHandle_t)this->m_handle);
        // give should always work
        FW_ASSERT(pdTRUE == stat,stat);
    }
            
}


