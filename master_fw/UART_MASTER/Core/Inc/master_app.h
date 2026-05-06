//Copyright 2026 Tobiasz_Kandziora
#ifndef MASTER_APP_H
#define MASTER_APP_H

#include <stdint.h>
#include "main.h"

void MasterApp_Init(void);
void MasterApp_Process(void);
void MasterApp_UART1_RxCallback(uint8_t rx_byte);
void MasterApp_UART_ErrorCallback(void);

#endif /* MASTER_APP_H */
