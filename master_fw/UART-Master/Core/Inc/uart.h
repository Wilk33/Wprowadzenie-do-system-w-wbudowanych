// Copyright 2026 TOK3T
/*
 * uart.h - UART/timer module
 */
#ifndef MASTER_FW_UART_MASTER_CORE_INC_UART_H_
#define MASTER_FW_UART_MASTER_CORE_INC_UART_H_

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void UART_Start(TIM_HandleTypeDef* htim, UART_HandleTypeDef* huart);
uint8_t UART_IsConnected(void);
void UART_SendLedValue(uint8_t value);
void UART_SetConnectionOk(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_H */
