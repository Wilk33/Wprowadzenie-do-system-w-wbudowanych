/* uart.h - UART/timer module
 */
#ifndef __UART_H
#define __UART_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void UART_Start(TIM_HandleTypeDef *htim, UART_HandleTypeDef *huart);
uint8_t UART_IsConnected(void);
void UART_SendLedValue(uint8_t value);
void UART_SetConnectionOk(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_H */
