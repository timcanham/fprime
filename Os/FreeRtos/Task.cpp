#include <Os/Task.hpp>
#include <Fw/Types/Assert.hpp>

#include <FreeRTOS.h>
#include <os_task.h>

typedef void* (*pthread_func_ptr)(void*);

STATIC bool s_schedStarted = false;

namespace Os {
    Task::Task() : m_handle(0), m_identifier(0), m_affinity(-1), m_started(false), m_suspendedOnPurpose(false) {
    }
    
    Task::TaskStatus Task::start(const Fw::StringBase &name, NATIVE_INT_TYPE identifier, NATIVE_INT_TYPE priority, NATIVE_INT_TYPE stackSize, taskRoutine routine, void* arg, NATIVE_INT_TYPE cpuAffinity) {

        this->m_name = "FR_";
        this->m_name += name;
        this->m_identifier = identifier;

        TaskHandle_t handle;

        BaseType_t stat = xTaskCreate( routine,                   /* The function that implements the task. */
                    name.toChar(),                                   /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                    stackSize,               /* The size of the stack to allocate to the task. */
                    arg,
                    priority ,        /* The priority assigned to the task. */
                    &handle );

        this->m_handle = reinterpret_cast<POINTER_CAST>(handle);

        if (stat != pdPASS) {
            return TASK_UNKNOWN_ERROR;
        }

        if (not s_schedStarted) {
            vTaskStartScheduler(); // only call once after first task is created
            s_schedStarted = true;
        }

        return TASK_OK;
    }
    
    Task::TaskStatus Task::delay(NATIVE_UINT_TYPE milliseconds)
    {
        
        TickType_t ticks = milliseconds/portTICK_PERIOD_MS;

        vTaskDelay(ticks);

        return TASK_OK; // for coverage analysis
        
    }


    Task::~Task() {
    	if (this->m_handle) {
    	    vTaskDelete(reinterpret_cast<TaskHandle_t>(this->m_handle));
    	}
        // If a registry has been registered, remove task
        if (Task::s_taskRegistry) {
            Task::s_taskRegistry->removeTask(this);
        }
        
    }

    // FIXME: Need to find out how to do this for Posix threads
    
    void Task::suspend(bool onPurpose) {
        FW_ASSERT(0);
    }
                    
    void Task::resume(void) {
        FW_ASSERT(0);
    }

    bool Task::isSuspended(void) {
        FW_ASSERT(0);
        return false;
    }

}

