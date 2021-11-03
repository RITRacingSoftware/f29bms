#ifndef HAL_UART_H
#define HAL_UART_H

#include <stdint.h>

/**
 * Initializes USART2 which is the uart connected to the usb port for the nucleo board
 */
void HAL_Uart_init(void);

/**
 * Send data over uart
 * buffer - pointer to a buffer of how much data to send
 * length - the length of the data to send (how many characters)
 */
void HAL_Uart_send(uint8_t *buffer, uint8_t length);

#endif // HAL_UART_H