// Copyright 2026 Gruby
#ifndef SLAVE_FW_SIL3_1_CORE_INC_SAFETY_LOGIC_H_
#define INC_SAFETY_LOGIC_H_

#include "uart_manager.h"

typedef enum {
  SL_STATE_IDLE,
  SL_STATE_WAIT_AUTH,
  SL_STATE_STOPPED,
  SL_STATE_ERROR
} SL_State_t;

typedef struct {
  SL_State_t state;
  uint32_t timer;
  uint8_t pendingLeds;
  UART_Manager_t *pcUart;
} SL_Context_t;

void SL_HandleMasterResponse(SL_Context_t *ctx, uint8_t masterResponse);
void SL_Init(SL_Context_t *ctx, UART_Manager_t *pcUart);
void SL_HandlePCCommand(SL_Context_t *ctx);
void SL_Update(SL_Context_t *ctx);

#endif  // SLAVE_FW_SIL3_1_CORE_INC_SAFETY_LOGIC_H_

