/** @file HL_sys_main.c 
*   @brief Application main file
*   @date 07-July-2017
*   @version 04.07.00
*
*   This file contains an empty main function,
*   which can be used for the application.
*/

/* 
* Copyright (C) 2009-2016 Texas Instruments Incorporated - www.ti.com  
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


/* USER CODE BEGIN (0) */
#include <stdio.h>
#include <FreeRTOS.h>
#include <os_task.h>
#include <os_queue.h>
#include <assert.h>
#include <HL_gio.h>
/* USER CODE END */

/* Include Files */

#include "HL_sys_common.h"

/* USER CODE BEGIN (1) */
/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */

static QueueHandle_t tickQueue = NULL;
static uint32 LEDState = 0;

static void tickTask( void *pvParameters ) {

    uint32_t val;
    gioSetBit(gioPORTB,6,1);

    for( ;; )
    {
        /* Wait until something arrives in the queue - this task will block
        indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
        FreeRTOSConfig.h. */
        xQueueReceive( tickQueue, &val, portMAX_DELAY );
        gioSetBit(gioPORTB,7,LEDState);
        LEDState = 1 - LEDState;
        //printf("Val: %d\n",val);
    }
}


/* USER CODE END */

uint8	emacAddress[6U] = 	{0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
uint32 	emacPhyAddress	=	1U;

int main(void)
{
/* USER CODE BEGIN (3) */

    gioInit();

    printf("Startup!\n");

    fsw_main();

//    tickQueue = xQueueCreate( 10, sizeof( unsigned long ) );
//
//    assert(tickQueue);
//
//    BaseType_t stat = xTaskCreate( tickTask,                   /* The function that implements the task. */
//                "ticker",                                   /* The text name assigned to the task - for debug only as it is not used by the kernel. */
//                configMINIMAL_STACK_SIZE,               /* The size of the stack to allocate to the task. */
//                0, /* The parameter passed to the task - just to check the functionality. */
//                tskIDLE_PRIORITY + 1 ,        /* The priority assigned to the task. */
//                NULL );                                 /* The task handle is not required, so NULL is passed. */
//
//    assert(stat == pdPASS);

    vTaskStartScheduler();
/* USER CODE END */

    return 0;
}


/* USER CODE BEGIN (4) */

uint32_t count = 0;
uint32_t divider = 10;

void vApplicationTickHook( void ) {
    count++;
    if (count % divider == 0) {
        xQueueSendFromISR( tickQueue, (void*)&count, NULL );
    }
}
/* USER CODE END */
