#include <stdbool.h>
#include "HAL_Uart.h"

#include "stm32f0xx_usart.h"
#include "stm32f0xx.h"
#include "stm32f0xx_gpio.h"

#define USART_PORT GPIOA
#define UART_PINS (GPIO_Pin_2 | GPIO_Pin_3)
#define UART_BAUD_RATE 115200

void HAL_Uart_init(void)
{
    //Enable clock
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    // setup gpio
    GPIO_InitTypeDef uartGPIOinit; 
    uartGPIOinit.GPIO_Pin = UART_PINS;
    uartGPIOinit.GPIO_Mode = GPIO_Mode_AF;
    uartGPIOinit.GPIO_OType = GPIO_OType_PP;
    uartGPIOinit.GPIO_Speed = GPIO_Speed_50MHz;
    uartGPIOinit.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &uartGPIOinit);

    // use pins A2 and A3 for USART tx/rx 
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_1);

    //Initialze the UART
    USART_InitTypeDef uart_init_struct;
    //Set values to default
    USART_StructInit(&uart_init_struct);
    uart_init_struct.USART_BaudRate = (uint32_t) UART_BAUD_RATE;
    USART_Init(USART2, &uart_init_struct);

    //Enable uart
    USART_Cmd(USART2, ENABLE);
}

void HAL_Uart_send(uint8_t *buffer, uint8_t length)
{
    bool data_left;
    uint8_t i = 0;
    // uint8_t i = 0;
    // if(length > 0)
    // {
    //     data_left = true;
    // }
    // else
    // {
    //     data_left = false;
    // }    
    // while(data_left)
    // {
    //     uint16_t next_data = 0;
    //     next_data |= ((buffer[i]) << 8);
    //     if(length > 1)
    //     {
    //         next_data |= (buffer[i+1]);
    //     }
    //     USART_SendData(USART2, next_data);
    //     i+=2;
    //     if(length > 2)
    //     {
    //         length = length - 2;
    //     }
    //     else
    //     {
    //         data_left = false;
    //     }
    // }
    while(i < (length))
    {
        uint16_t next_data = buffer[i];
        while((USART2->ISR & 0x40) == 0);
        USART_SendData(USART2, next_data);
        i++;
    }
}