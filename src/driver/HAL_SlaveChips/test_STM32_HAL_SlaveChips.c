#include "HAL_SlaveChips.h"
#include "HAL_Clock.h"
#include "stm32f0xx_gpio.h"

//Temporary fix to get it to build
#include "FreeRTOS.h"
#include "semphr.h"
SemaphoreHandle_t can_message_recieved_semaphore;
SemaphoreHandle_t can_message_transmit_semaphore;

#define HSI48_SOURCE 0x0C


int main()
{
        // enable HSI48 clock
    // RCC_HSI48Cmd(ENABLE);
    // // wait for HSI48 clock to be ready
    // while(!RCC_GetFlagStatus(RCC_FLAG_HSI48RDY));
    // // tell system clock (which CAN uses) to use HSI48 clock
    // RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI48);
    // // wait for system clock to switch over to HSI48
    // while(RCC_GetSYSCLKSource() != HSI48_SOURCE);
    // // enable GPIOA and CAN peripherals
    // RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN, ENABLE);
    // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    HAL_Clock_init();

    HAL_SlaveChips_init();
    int num_ltcs = 2; //Number of LTC chips
    int num_boards = num_ltcs / 2; // note this only works for an even number of chips
    bool setIsDraining[num_boards*15]; 

    // to set all on
    // for (int i = 0; i < num_boards*15;i++)
    // {
    //     if (i % 2)
    //     setIsDraining[i] = 1;
    //     else
    //     setIsDraining[i] = 0;
    // }
    // HAL_SlaveChips_request_cell_drain_state(setIsDraining, sizeof(setIsDraining)/sizeof(setIsDraining[0]));
    // for(;;);
    
    for(int i = 0; i < num_boards*15; i++) setIsDraining[i] = 0;
    for(;;)
    {
    
    for (int iter = 0; iter < num_boards* 15;iter++)
    {

        //if (iter != 0) setIsDraining[iter-1] = 0;
        setIsDraining[iter] = 1;
               // HAL_SlaveChips_request_cell_drain_state(setIsDraining, sizeof(setIsDraining)/sizeof(setIsDraining[0]));

        HAL_SlaveChips_request_cell_drain_state(setIsDraining, sizeof(setIsDraining)/sizeof(setIsDraining[0]));
        for(int lol = 0; lol < 80000; lol++);
    }
    for (int iter = num_boards*15-1; iter >=0;iter--)
    {

        //if (iter != 0) setIsDraining[iter-1] = 0;
        setIsDraining[iter] = 0;
        //HAL_SlaveChips_request_cell_drain_state(setIsDraining, sizeof(setIsDraining)/sizeof(setIsDraining[0]));

        HAL_SlaveChips_request_cell_drain_state(setIsDraining, sizeof(setIsDraining)/sizeof(setIsDraining[0]));
        for(int lol = 0; lol < 80000; lol++);
    }

    // for (int i = 0; i < num_boards*15;i++)
    // {
    //     if (i % 2)
    //     setIsDraining[i] = 1;
    //     else
    //     setIsDraining[i] = 0;
    // }
    // HAL_SlaveChips_request_cell_drain_state(setIsDraining, sizeof(setIsDraining)/sizeof(setIsDraining[0]));
    //         HAL_SlaveChips_request_cell_drain_state(setIsDraining, sizeof(setIsDraining)/sizeof(setIsDraining[0]));

    //      for(int lol = 0; lol < 800000; lol++);
    //       for (int i = 0; i < num_boards*15;i++)
    // {
    //     if (i % 2)
    //     setIsDraining[i] = 0;
    //     else
    //     setIsDraining[i] = 1;
    // }
    // HAL_SlaveChips_request_cell_drain_state(setIsDraining, sizeof(setIsDraining)/sizeof(setIsDraining[0]));
    //          HAL_SlaveChips_request_cell_drain_state(setIsDraining, sizeof(setIsDraining)/sizeof(setIsDraining[0]));

    //      for(int lol = 0; lol < 800000; lol++);
    }
    
    // int cv[24];

    // for (int i = 0; i < 24; i++)
    // {
    //     cv[i] = (int)(cellVoltages[i] * 1000.0);
    // }
    
    // bool isDraining[num_ltcs * 12];
    // for (int i = 0; i< num_ltcs*12; i++)
    // {
    //     isDraining[i] = 0;
    // }

    // float cellVoltages[num_ltcs * 12];
    // HAL_SlaveChips_get_all_cell_data(&cellVoltages[0], isDraining, (num_ltcs*12));
    
    // float temps[(num_ltcs/2)*3];
    // HAL_SlaveChips_get_all_tm_readings(temps, 3);

    return 0;
}