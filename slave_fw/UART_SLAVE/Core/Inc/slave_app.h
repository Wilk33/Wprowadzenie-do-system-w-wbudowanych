//Copyright 2026 Tobiasz_Kandziora
#ifndef SLAVE_APP_H
#define SLAVE_APP_H

#include <stdint.h>

void SlaveApp_Init(void);
void SlaveApp_Process(void);
void SlaveApp_UART1_RxCallback(uint8_t rx_byte);
void SlaveApp_UART2_RxCallback(uint8_t rx_byte);

#endif /* SLAVE_APP_H */
