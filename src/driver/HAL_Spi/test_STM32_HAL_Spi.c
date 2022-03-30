#include "HAL_Spi.h"

//Temporary fix to get it to build
#include "FreeRTOS.h"
#include "semphr.h"
SemaphoreHandle_t can_message_recieved_semaphore;
SemaphoreHandle_t can_message_transmit_semaphore;


int main(){
    HAL_Spi_init();
    char tx[2] = {'6', '9'};
    char rx[2] = {0,0};
    for(;;)
    {
        HAL_Spi_transmit_and_receive(tx, 2, rx, 2);
        for(int i = 0; i < 10000000; i++);
    }
    
    return 0;
}