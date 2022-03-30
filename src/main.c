// Kernel includes.
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#ifdef SIMULATION
#include "BmsSimClient.h"
#endif

#include <stdio.h>
#include <string.h>

#include "HAL_Clock.h"
#include "HAL_Can.h"
#include "HAL_Gpio.h"
#include "HAL_CurrentSensor.h"
#include "HAL_EEPROM.h"
#include "HAL_Watchdog.h"
#include "HAL_SlaveChips.h"
#include "HAL_Uart.h"

#include "f29BmsConfig.h"

#include "BatteryModel.h"
#include "CAN.h"
#include "CellBalancer.h"
#include "ChargeMonitor.h"
#include "CurrentMonitor.h"
#include "CurrentSense.h"
#include "DriveMonitor.h"
#include "FaultManager.h"
#include "NTCALUG01T_LUT.h"
#include "PackMonitor.h"
#include "Periodic.h"
#include "SlaveInterface.h"
#include "SOCestimator.h"
#include "StatusLed.h"
#include "TempConverter.h"
#include "TempModel.h"
#include "TaskWatchdog.h"
#include "semphr.h"

// #define DISABLE_WATCHDOG

#define SEPHAMORE_WAIT 0

SemaphoreHandle_t can_message_recieved_semaphore;
SemaphoreHandle_t can_message_transmit_semaphore;

#define TASK_1Hz_NAME "task_1Hz"
#define TASK_1Hz_PRIORITY (tskIDLE_PRIORITY + 1)
#define TASK_1Hz_PERIOD_MS (1000)
#define TASK_1Hz_STACK_SIZE_B (1700)
void TASK_1Hz(void *pvParameters)
{
    (void) pvParameters;
    TickType_t next_wake_time = xTaskGetTickCount();

    for (;;)
    {
        Periodic_1Hz();
        //Don't use watchdog if Disabled
        #ifndef DISABLE_WATCHDOG
            TaskWatchdog_pet(task_id_PERIODIC_1Hz);
        #endif
        
        // printf("Task 1Hz\n");
        vTaskDelayUntil(&next_wake_time, TASK_1Hz_PERIOD_MS);
    }
}

#define TASK_10Hz_NAME "task_10Hz"
#define TASK_10Hz_PRIORITY (tskIDLE_PRIORITY + 1)
#define TASK_10Hz_PERIOD_MS (100)
#define TASK_10Hz_STACK_SIZE_B (1000)
void task_10Hz(void *pvParameters)
{
    (void) pvParameters;
    TickType_t next_wake_time = xTaskGetTickCount();
    for (;;)
    {
        Periodic_10Hz();
        //Don't use watchdog if Disabled
        #ifndef DISABLE_WATCHDOG
            TaskWatchdog_pet(task_id_PERIODIC_10Hz);
        #endif
        // printf("Task 10Hz\n");
        
        vTaskDelayUntil(&next_wake_time, TASK_10Hz_PERIOD_MS);
    }
}

#define TASK_1kHz_NAME "task_1kHz"
#define TASK_1kHz_PRIORITY (tskIDLE_PRIORITY + 3)
#define TASK_1kHz_PERIOD_MS (1)
#define TASK_1kHz_STACK_SIZE_B (1000)

void task_1kHz(void *pvParameters)
{
    (void) pvParameters;
    TickType_t next_wake_time = xTaskGetTickCount();
    for (;;)
    {
        Periodic_1kHz();
        //Don't use watchdog if Disabled
        #ifndef DISABLE_WATCHDOG
            TaskWatchdog_pet(task_id_PERIODIC_1kHz);
        #endif
        //printf("Task 1kHz\n");
        vTaskDelayUntil(&next_wake_time, TASK_1kHz_PERIOD_MS);
    }
}

#define TASK_CAN_RX_NAME "task_CAN_RX"
#define TASK_CAN_RX_PRIORITY (tskIDLE_PRIORITY + 4)
#define TASK_CAN_RX_PERIOD_MS (1)
#define TASK_CAN_RX_STACK_SIZE_B (500) 

void task_can_rx(void *pvParameters)
{
    (void) pvParameters;
    // TickType_t next_wake_time = xTaskGetTickCount();
    for (;;)
    {
        if(xSemaphoreTake(can_message_recieved_semaphore, portMAX_DELAY) == pdTRUE)
        {
            CAN_process_recieved_messages();
        }
    }
}

#define TASK_CAN_TX_NAME "task_CAN_TX"
#define TASK_CAN_TX_PRIORITY (tskIDLE_PRIORITY + 4)
#define TASK_CAN_TX_PERIOD_MS (1)
#define TASK_CAN_TX_STACK_SIZE_B (500) 

void task_can_tx(void *pvParameters)
{
    (void) pvParameters;
    // TickType_t next_wake_time = xTaskGetTickCount();
    for (;;)
    {
        if(xSemaphoreTake(can_message_transmit_semaphore, portMAX_DELAY) == pdTRUE)
        {
            CAN_send_queued_messages();
        }
    }
}


#define WATCHDOG_TASK_NAME ((signed char *) "watchdog_task")
#define WATCHDOG_TASK_STACK_SIZE (256) //100
#define WATCHDOG_TASK_PERIOD (1)
#define WATCHDOG_TASK_PRIORITY (tskIDLE_PRIORITY+2) //Not sure 
void watchdog_task(void *pvParameters)
{
    (void)pvParameters;
	TickType_t next_wake_time = xTaskGetTickCount();
	for(;;)
	{
        //Don't use watchdog if Disabled
        #ifndef DISABLE_WATCHDOG
		if (!TaskWatchdog_expired())
		{
			HAL_Watchdog_pet();

			TaskWatchdog_tick(task_id_PERIODIC_1Hz);
			TaskWatchdog_tick(task_id_PERIODIC_10Hz);
            TaskWatchdog_tick(task_id_PERIODIC_1kHz);
		}
        #endif

		vTaskDelayUntil(&next_wake_time, WATCHDOG_TASK_PERIOD);
	}
}



#ifdef SIMULATION
#include <signal.h>
#include <stdlib.h>
void signal_handler(int signo)
{
    if (signo == SIGHUP || signo == SIGINT)
    {
        BmsSimClient_close();
        exit(0);
    }
}
#endif

void hardfault_handler_routine(void)
{
    // shut down car
    HAL_Gpio_write(GpioPin_SHUTDOWN_LINE, 0);

    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    char* task_name = pcTaskGetName(current_task);

    int task_id = 0;
    if (strcmp(task_name, TASK_1Hz_NAME) == 0)
    {
        task_id = 1;
    }
    else if (strcmp(task_name, TASK_10Hz_NAME) == 0)
    {
        task_id = 2;
    }
    else if (strcmp(task_name, TASK_1kHz_NAME) == 0)
    {
        task_id = 3;
    }
    else if (strcmp(task_name, TASK_CAN_RX_NAME) == 0)
    {
        task_id = 4;
    }

    uint8_t data[8];
    can_bus.bms_hard_fault_indicator.bms_hard_fault_indicator_task = task_id;
    f29bms_dbc_bms_hard_fault_indicator_pack(data, &can_bus.bms_hard_fault_indicator, 8);
    HAL_Can_send_message(F29BMS_DBC_BMS_HARD_FAULT_INDICATOR_FRAME_ID, 8, *((uint64_t*)data)); 
}

int main(int argc, char** argv)
{
    // initialize all HAL stuff
    HAL_Clock_init();
    
    HAL_Gpio_init(); // must happen before CAN
    // HAL_Uart_init();
    
    can_message_recieved_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(can_message_recieved_semaphore);
    xSemaphoreTake(can_message_recieved_semaphore, SEPHAMORE_WAIT);
    can_message_transmit_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(can_message_transmit_semaphore);
    xSemaphoreTake(can_message_transmit_semaphore, SEPHAMORE_WAIT);
    
    HAL_Can_init();
   
    HAL_CurrentSensor_init();
   
    HAL_SlaveChips_init();

    #ifndef DISABLE_WATCHDOG
        HAL_Watchdog_init();
    #endif

    // initialize all app stuff
    CAN_init();
    
    CellBalancer_init();
    ChargeMonitor_init();
    CurrentMonitor_init();
    CurrentSense_init();
    DriveMonitor_init();
    PackMonitor_init();
    Periodic_init();
    SOCestimator_init();
    FaultManager_init();
    StatusLed_init();
    TaskWatchdog_init();
    TempConverter_init(NTCALUG01T_LUT, NTCALUG01T_LUT_LEN, NTCALUG01T_OFFSET, DIVIDER_OHM);


#ifdef SIMULATION
    if (BmsSimClient_init())
    {
        printf("Could not init bmssimclient\n");
        return -1;
    }

     signal(SIGHUP, signal_handler);
     signal(SIGINT, signal_handler);
#endif

 

    xTaskCreate(TASK_1Hz, 
        TASK_1Hz_NAME, 
        TASK_1Hz_STACK_SIZE_B,
        NULL,
        TASK_1Hz_PRIORITY,
        NULL);

    
    xTaskCreate(task_10Hz, 
        TASK_10Hz_NAME, 
        TASK_10Hz_STACK_SIZE_B,
        NULL,
        TASK_10Hz_PRIORITY,
        NULL);



    xTaskCreate(task_1kHz, 
        TASK_1kHz_NAME, 
        TASK_1kHz_STACK_SIZE_B,
        NULL,
        TASK_1kHz_PRIORITY,
        NULL);

    xTaskCreate(task_can_rx, 
        TASK_CAN_RX_NAME, 
        TASK_CAN_RX_STACK_SIZE_B,
        NULL,
        TASK_CAN_RX_PRIORITY,
        NULL);

    xTaskCreate(task_can_tx, 
        TASK_CAN_TX_NAME, 
        TASK_CAN_TX_STACK_SIZE_B,
        NULL,
        TASK_CAN_TX_PRIORITY,
        NULL);

    xTaskCreate(watchdog_task,
        WATCHDOG_TASK_NAME,
        WATCHDOG_TASK_STACK_SIZE,
        NULL,
        WATCHDOG_TASK_PRIORITY,
        NULL);
    
    
    vTaskStartScheduler();

    // if we get here, ope
    for (;;);
}