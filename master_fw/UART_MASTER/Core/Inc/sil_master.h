// Copyright 2026 Tobiasz_Kandziora
#ifndef MASTER_FW_UART_MASTER_CORE_INC_SIL_MASTER_H_
#define SIL_MASTER_H

#include <stdint.h>

void SIL_Master_Init(void);
void SIL_Master_Process(void);
uint16_t SIL_CalculateCRC(uint8_t *data, uint16_t length);

#endif  // MASTER_FW_UART_MASTER_CORE_INC_SIL_MASTER_H_
