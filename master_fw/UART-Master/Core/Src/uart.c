// Copyright 2026 TOK3T
/*
 *uart.c - UART and timer logic
 */

#include "uart.h"

#include "leds.h"
#include "stm32l4xx_hal.h"


static TIM_HandleTypeDef* s_htim = NULL;
static UART_HandleTypeDef* s_huart = NULL;

/* connection state: 0=none, 1=ok, 2=waiting */
static volatile uint8_t s_connection_state = 1;
static volatile uint8_t s_pong_byte = 0;

void UART_Start(TIM_HandleTypeDef* htim, UART_HandleTypeDef* huart) {
    s_htim = htim;
    s_huart = huart;
}

uint8_t UART_IsConnected(void) {
    return (s_connection_state != 0);
}

void UART_SetConnectionOk(void) {
    s_connection_state = 1;
    /* ensure receiver is armed */
    if (s_huart)
        HAL_UART_Receive_IT(s_huart, (uint8_t*)&s_pong_byte, 1);  // NOLINT
}

void UART_SendLedValue(uint8_t value) {
    if (!s_huart) return;
    HAL_UART_Transmit(s_huart, (uint8_t*)&value, 1, 10);  // NOLINT
}

/* HAL callbacks handled here */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim->Instance == TIM2) {
        static uint8_t fail_count = 0;

        if (s_connection_state == 2) {
            fail_count++;
            if (fail_count >= 1) {
                s_connection_state = 0;
                if (s_huart) HAL_UART_AbortReceive(s_huart);
                Led_OnDisconnect();
            }
        }

        if (s_connection_state != 0) {
            s_connection_state = 2;
            uint8_t ping = 0xFF;
            if (s_huart) {
                HAL_UART_Receive_IT(s_huart, (uint8_t*)&s_pong_byte,
                                    1); // NOLINT(readability/casting)
                HAL_UART_Transmit(s_huart, &ping, 1, 10);
            }
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
    if (s_huart && huart->Instance == s_huart->Instance) {
        if (s_pong_byte == 0xFF) {
            s_connection_state = 1;
        }
        if (s_huart)
            HAL_UART_Receive_IT(s_huart, (uint8_t*)&s_pong_byte, 1);  // NOLINT
    }
}
