// Copyright 2026 Gruby
#ifndef SLAVE_FW_SIL3_1_CORE_INC_MASTER_COMM_H_
#define INC_MASTER_COMM_H_

#include "main.h"
#include <stdbool.h>

#define MASTER_GRANT 0xA5
#define MASTER_DENY 0x5A

void MC_SendAuthRequest(uint8_t ledCount);  //NOLINT
bool MC_CheckResponse(
    uint8_t *response);
void MC_IRQHandler(
    UART_HandleTypeDef *huart);

#endif  // SLAVE_FW_SIL3_1_CORE_INC_MASTER_COMM_H_
