#include "usart.h"

__attribute__((used)) int _write(int file, char* ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, len);

    return len;
}