#include "HAL_Watchdog.h"
#include "stm32f0xx_wwdg.h"
#include "HAL_Can.h"
#include "core_cm0.h"
#include "stm32f0xx.h"
#include "CAN.h"

#define CAN_RESET_DLC 8


#define WINDOW_VALUE 0x7F //Don't know what should be
#define SET_VALUE 0x7F //Value counter is reset to
//MCU resets when counter gets down to 0x40

//t(watchdog) = (tclk) * 4096 * 2^(prescale) * 64
//Ex: clk = 48MHz, prescale: WWD_PRESCALER_8 (2^3 = 8)
//t = (1/48000000) * 4096 * 8 * 64 = 0.04369 s = 43.69 ms
//For 48MHz, max timeout time ~43 ms, min timeout time ~85us

void HAL_Watchdog_init(){
    


    //Right now set to 43ms timeout (Prescaler = 8 assuming 48MHz clk)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_DBGMCU, ENABLE);

    //Set so WWDG stops during breakpoint in debug
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_WWDG_STOP;

    WWDG_SetPrescaler(WWDG_Prescaler_8); //Can be 1,2,4,8 
    WWDG_SetWindowValue(WINDOW_VALUE);
    WWDG_Enable(SET_VALUE);
    

    /* (1) Set prescaler to have a roll-over each about 5.5ms,
    set window value (about 2.25ms) */
    /* (2) Refresh WWDG before activate it */
    /* (3) Activate WWDG */
    // WWDG->CFR = SET_VALUE; /* (1) */
    // WWDG->CR = WWDG_REFRESH; /* (2) */
    // WWDG->CR |= WWDG_CR_WDGA; /* (3) */
}

void HAL_Watchdog_pet(){
    WWDG_SetCounter(SET_VALUE);
    //WWDG->CR = SET_VALUE & BIT_MASK;
}


//MAke function for reset handler resets the chip, put in startup_stm32f0xx.s
void HAL_Watchdog_IRQHandler(void){

    
    //MCU Reset
    NVIC_SystemReset();

}