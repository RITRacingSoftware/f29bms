#include "HAL_Uart.h"

int main(){
    HAL_Uart_init();
    int8_t print_buffer[10];
    uint8_t n = sprintf(&print_buffer[0], "abcd\n\r");
    HAL_Uart_send(&print_buffer[0], n);
    n = sprintf(&print_buffer[0], "Hello\n\r");
    HAL_Uart_send(&print_buffer[0], n);
}