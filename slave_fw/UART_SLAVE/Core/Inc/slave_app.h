// Copyright 2026 Tobiasz_Kandziora
// Ten plik zawiera funkcje odpowiedzialne za główną maszynę stanów Slave'a
// i odbieranie komend z UART.
#ifndef SLAVE_FW_UART_SLAVE_CORE_INC_SLAVE_APP_H_
#define SLAVE_FW_UART_SLAVE_CORE_INC_SLAVE_APP_H_

#include <stdint.h>

void SlaveApp_Init(void);
void SlaveApp_Process(void);
void SlaveApp_UART1_RxCallback(uint8_t rx_byte);
void SlaveApp_UART2_RxCallback(uint8_t rx_byte);

#endif  // SLAVE_FW_UART_SLAVE_CORE_INC_SLAVE_APP_H_
