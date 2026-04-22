// Copyright 2026 Gruby
#include "master_comm.h"
#include "usart.h"
#include <stdbool.h>

// Zakładamy, że Master jest na huart1
extern UART_HandleTypeDef huart1;
static uint8_t rxByte;
static bool dataReady = false;

void MC_SendAuthRequest(uint8_t ledCount) {
  uint8_t frame[3] = {0x3A, ledCount, 0x0D}; // Prosta ramka binarna
  dataReady = false;
  HAL_UART_Transmit(&huart1, frame, 3, 10);
  // Uzbrajamy odbiór odpowiedzi
  HAL_UART_Receive_IT(&huart1, &rxByte, 1);
}

// Wywoływane w HAL_UART_RxCpltCallback dla huart1
void MC_IRQHandler(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    dataReady = true;
  }
}

bool MC_CheckResponse(uint8_t *response) {
  if (dataReady) {
    *response = rxByte;
    dataReady = false;
    return true;
  }
  return false; // Tego returna brakowało w linii 31
}
