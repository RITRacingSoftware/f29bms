#ifndef HAL_WATCHDOG_H
#define HAL_WATCHDOG_H

// This header just exists for Mocks. TODO- implement this

void HAL_Watchdog_init(void);

void HAL_Watchdog_pet(void);

void HAL_Watchdog_IRQHandler(void);

#endif // HAL_WATCHDOG_H