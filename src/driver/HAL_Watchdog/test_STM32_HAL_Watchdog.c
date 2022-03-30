#include "HAL_Watchdog.h"
#include "stm32f0xx_rcc.h"

//Temporary fix to get it to build
#include "FreeRTOS.h"
#include "semphr.h"
SemaphoreHandle_t can_message_recieved_semaphore;
SemaphoreHandle_t can_message_transmit_semaphore;

#define SYS_CLK 48000000
#define TIMER_FREQUENCY 1000 //1 KHZ (1 ms period)
#define DELAY 44 //in ms //35

#define HSI48_SOURCE 0x0C

int main()
{
   //Test on full program (added to 1KHz task)
   // enable HSI48 clock
    RCC_HSI48Cmd(ENABLE);
    // wait for HSI48 clock to be ready
    while(!RCC_GetFlagStatus(RCC_FLAG_HSI48RDY));
    // tell system clock (which CAN uses) to use HSI48 clock
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI48);
    // wait for system clock to switch over to HSI48
    while(RCC_GetSYSCLKSource() != HSI48_SOURCE);
    HAL_Watchdog_init();
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->PSC = SYS_CLK/TIMER_FREQUENCY;  //Timer_F = SYS/Pre
    TIM6->CR1 |= (TIM_CR1_ARPE | TIM_CR1_URS | TIM_CR1_DIR);
    HAL_Watchdog_pet();
    while(1==1){
        TIM6->ARR = DELAY; 
        TIM6->EGR |= TIM_EGR_UG;
        TIM6->SR = 0;
        TIM6->CR1 |= TIM_CR1_CEN;
        while(TIM6->SR == 0);
        HAL_Watchdog_pet();
    }
}