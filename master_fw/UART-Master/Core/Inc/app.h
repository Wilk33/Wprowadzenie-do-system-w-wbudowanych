/*
 * app.h
 * Declarations for application-level logic moved out of main.c
 */
#ifndef __APP_H
#define __APP_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Connection state values */
#define CONN_NONE 0U
#define CONN_OK   1U
#define CONN_WAIT 2U

/* Public API */
void App_Init(void);
void App_Start(TIM_HandleTypeDef *htim, UART_HandleTypeDef *huart);

void Update_LEDs(uint8_t count);

/* Exposed state (if other modules need to read) */
extern volatile uint8_t led_number;
extern volatile uint8_t connection_state;
extern volatile uint8_t pong_byte;
extern volatile uint32_t last_button_press_time;
extern const uint32_t DEBOUNCE_DELAY;

#ifdef __cplusplus
}
#endif

#endif /* __APP_H */
