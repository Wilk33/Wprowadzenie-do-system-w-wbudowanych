// Copyright 2026 TOK3T
/* leds.h
 * LED control module
 */
#ifndef MASTER_FW_UART_MASTER_CORE_INC_LEDS_H_
#define MASTER_FW_UART_MASTER_CORE_INC_LEDS_H_

#include <stdint.h>

#include "main.h"


#ifdef __cplusplus
extern "C" {
#endif

void Led_Init(void);
void Update_LEDs(uint8_t count);
uint8_t Led_GetNumber(void);
void Led_OnDisconnect(void);

#ifdef __cplusplus
}
#endif

#endif  // MASTER_FW_UART_MASTER_CORE_INC_LEDS_H_
