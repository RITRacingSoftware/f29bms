#include "HAL_Gpio.h"

//Temporary fix to get it to build
#include "FreeRTOS.h"
#include "semphr.h"
SemaphoreHandle_t can_message_recieved_semaphore;
SemaphoreHandle_t can_message_transmit_semaphore;

int main()
{
    HAL_Gpio_init();
    for(;;)
    {
        bool current_state = HAL_Gpio_read(GpioPin_STATUS_LED);
        HAL_Gpio_write(GpioPin_STATUS_LED, current_state ^ 1);
        for (int i = 0; i < 500000; i++);
        
    }
}