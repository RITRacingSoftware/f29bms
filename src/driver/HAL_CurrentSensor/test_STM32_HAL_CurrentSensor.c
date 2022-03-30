#include "stm32f0xx_adc.h"
#include "HAL_CurrentSensor.h"

//Temporary fix to get it to build
#include "FreeRTOS.h"
#include "semphr.h"
SemaphoreHandle_t can_message_recieved_semaphore;
SemaphoreHandle_t can_message_transmit_semaphore;

int main()
{
    HAL_CurrentSensor_init();

    while(1)
    {
        static float current;
        HAL_CurrentSensor_read_current(&current);
    }
}