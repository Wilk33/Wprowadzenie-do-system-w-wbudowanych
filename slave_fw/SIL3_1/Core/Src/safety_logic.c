// Copyright 2026 Gruby
#include "safety_logic.h"
#include "cmd_parser.h"
#include "io_manager.h"
#include "master_comm.h"


#define AUTH_TIMEOUT 1000 // 1 sekunda

void SL_Init(SL_Context_t *ctx, UART_Manager_t *pcUart) {
  ctx->state = SL_STATE_IDLE;
  ctx->pcUart = pcUart;
  IO_Init();
  IO_AllOff();
}

void SL_HandlePCCommand(SL_Context_t *ctx) {
  CP_Result_t res = CP_Parse(UARTM_GetToString(ctx->pcUart));

  if (res.type == CMD_STOP) {
    ctx->state = SL_STATE_STOPPED;
    IO_AllOff();
    UARTM_SendContinuous(ctx->pcUart, "STATUS: STOPPED_SAFE");
    return;
  }

  if (res.type == CMD_START) {
    IO_RunSelfTest();
    ctx->state = SL_STATE_IDLE;
    UARTM_SendContinuous(ctx->pcUart, "STATUS: READY");
    return;
  }

  if (res.type == CMD_LED_SET &&
      (ctx->state == SL_STATE_IDLE || ctx->state == SL_STATE_STOPPED)) {
    ctx->pendingLeds = res.ledCount;
    MC_SendAuthRequest(res.ledCount);
    ctx->timer = HAL_GetTick();
    ctx->state = SL_STATE_WAIT_AUTH;
    UARTM_SendContinuous(ctx->pcUart, "STATUS: AWAITING_MASTER");
  } else if (res.type == CMD_INVALID) {
    UARTM_SendContinuous(ctx->pcUart, "ERR: INVALID_SYNTAX");
  }
}

void SL_Update(SL_Context_t *ctx) {
  if (ctx->state == SL_STATE_WAIT_AUTH) {
    uint8_t response;
    if (MC_CheckResponse(&response)) {
      if (response == MASTER_GRANT) {
        IO_SetLeds(ctx->pendingLeds);
        ctx->state = SL_STATE_IDLE;
        UARTM_SendContinuous(ctx->pcUart, "OK: EXECUTED");
      } else {
        ctx->state = SL_STATE_IDLE;
        UARTM_SendContinuous(ctx->pcUart, "ERR: MASTER_DENIED");
      }
    } else if (HAL_GetTick() - ctx->timer > AUTH_TIMEOUT) {
      ctx->state = SL_STATE_ERROR;
      IO_AllOff();
      UARTM_SendContinuous(ctx->pcUart, "ERR: MASTER_TIMEOUT_EMERGENCY");
    }
  }
}
void SL_HandleMasterResponse(SL_Context_t *ctx, uint8_t masterResponse) {
  // Sprawdzamy, czy w ogóle czekaliśmy na autoryzację
  if (ctx->state == SL_STATE_WAIT_AUTH) {
    if (masterResponse == MASTER_GRANT) {
      // Master pozwolił - przechodzimy do wykonania (zapalenia diod w
      // SL_Update) W naszej maszynie stanów SL_Update zobaczy zmianę i wywoła
      // IO_SetLeds
      ctx->state = SL_STATE_IDLE; // Lub dedykowany stan EXECUTING
      IO_SetLeds(ctx->pendingLeds);
      UARTM_SendContinuous(ctx->pcUart, "OK: AUTHORIZED BY MASTER");
    } else {
      // Master odmówił
      ctx->state = SL_STATE_IDLE;
      IO_AllOff();
      UARTM_SendContinuous(ctx->pcUart, "ERR: MASTER DENIED ACCESS");
    }
  }
}
