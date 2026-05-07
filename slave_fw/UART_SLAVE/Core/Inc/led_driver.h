// Copyright 2026 Tobiasz_Kandziora
// Tutaj znajdują się tylko funkcje do fizycznego zapalania diod i robienia Self-Testu.
#ifndef SLAVE_FW_UART_SLAVE_CORE_INC_LED_DRIVER_H_
#define SLAVE_FW_UART_SLAVE_CORE_INC_LED_DRIVER_H_

#include <stdint.h>

void LED_Set(uint8_t status);
void LED_SelfTest_Start(void);
void LED_SelfTest_Update(void);
void LED_SelfTest_Abort(void);
int LED_IsTesting(void);

#endif  // SLAVE_FW_UART_SLAVE_CORE_INC_LED_DRIVER_H_
