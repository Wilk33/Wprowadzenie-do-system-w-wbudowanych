// Copyright 2026 Tobiasz_Kandziora
#include "master_app.h" // NOLINT
#include "sil_master.h" // NOLINT
#include "stm32l4xx_hal.h" // NOLINT
#include <stdio.h> // NOLINT
#include <string.h> // NOLINT
#include <stdlib.h> // NOLINT

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

#define AUTH_TIMEOUT_MS 5000
#define RX_BUFFER_SIZE 20

typedef enum {
  MASTER_IDLE,
  MASTER_PROCESSING_AUTH,
  MASTER_AUTHORIZATION_DONE
} MasterState_t;

static MasterState_t master_state = MASTER_IDLE;
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint8_t rx_index = 0;
static uint32_t auth_start_time = 0;
static char last_command = '0';
static uint8_t authorization_response = 0;

/* --- Funkcje prywatne --- */

static void SendAuthorizationResponse(uint8_t response) {
  char response_msg[10];
  snprintf(response_msg, sizeof(response_msg), "%d\r\n", response);

  HAL_UART_Transmit(&huart2, (uint8_t*) "SENDING_AUTH:", 13, 100);  // NOLINT
  HAL_UART_Transmit(&huart2, (uint8_t*) response_msg, strlen(response_msg),
                    100);  // NOLINT
  HAL_UART_Transmit(&huart1, (uint8_t*) response_msg, strlen(response_msg),
                    100);  // NOLINT
}

static void ProcessAuthorizationRequest(int command) {
  if (command >= 1 && command <= 8) {
    last_command = command;
    authorization_response = 1;
    master_state = MASTER_AUTHORIZATION_DONE;
    SendAuthorizationResponse(1);
  } else {
    authorization_response = 0;
    master_state = MASTER_AUTHORIZATION_DONE;
    SendAuthorizationResponse(0);
  }
}

static void HandleAuthorizationTimeout(void) {
  if (master_state == MASTER_PROCESSING_AUTH) {
    if ((HAL_GetTick() - auth_start_time) > AUTH_TIMEOUT_MS) {
      authorization_response = 0;
      master_state = MASTER_IDLE;
      rx_index = 0;
      HAL_UART_Transmit(&huart1, (uint8_t*) "Authorization timeout\r\n", 23,
                        100);  // NOLINT
    }
  }
}

/* --- Zewnętrzne API (wywoływane w main.c) --- */

void MasterApp_Init(void) {
  SIL_Master_Init();
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
  master_state = MASTER_IDLE;
  rx_index = 0;
  memset(rx_buffer, 0, RX_BUFFER_SIZE);
}

void MasterApp_Process(void) {
  /* Utrzymanie znaku życia dla Slave'a */
  SIL_Master_Process();

  /* Sprawdzenie timeoutu autoryzacji */
  HandleAuthorizationTimeout();

  if (master_state == MASTER_AUTHORIZATION_DONE) {
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    master_state = MASTER_IDLE;
  }

  HAL_Delay(10); /* Lekki oddech dla procesora */
}

void MasterApp_UART1_RxCallback(uint8_t rx_byte) {
  if (rx_index < RX_BUFFER_SIZE - 1) {
    rx_buffer[rx_index++] = rx_byte;
  }

  /* Wykrycie końca linii */
  if (rx_byte == '\n') {
    /* Czyszczenie białych znaków na końcu */
    for (int i = 0; i < rx_index; i++) {
      if (rx_buffer[i] == '\r' || rx_buffer[i] == '\n') {
        rx_buffer[i] = '\0';
        break;
      }
    }

    /* Parsowanie autoryzacji */
    if (strncmp((char*) rx_buffer, "Aut_req:", 8) == 0) {  // NOLINT
      int command = 0;
      uint16_t received_crc = 0;

      char *ptr = (char*) &rx_buffer[8];  // NOLINT
      command = atoi(ptr);

      while (*ptr && *ptr != ',')
        ptr++;
      if (*ptr == ',')
        ptr++;
      received_crc = (uint16_t) strtol(ptr, NULL, 16);

      if (command > 0 && command <= 8) {
        uint8_t cmd_byte = (uint8_t) command;
        uint16_t calculated_crc = SIL_CalculateCRC(&cmd_byte, 1);

        if (calculated_crc == received_crc) {
          HAL_UART_Transmit(&huart2, (uint8_t*) "CRC_OK - PROCESSING\r\n", 21,
                            100);
          ProcessAuthorizationRequest(command);  // NOLINT
        } else {
          HAL_UART_Transmit(&huart2, (uint8_t*) "CRC_FAIL\r\n", 10, 100);  // NOLINT
          SendAuthorizationResponse(0);
        }
      }
    } else {
      HAL_UART_Transmit(&huart2, (uint8_t*) "FMT_ERR\r\n", 9, 100);  // NOLINT
    }

    /* Reset bufora na kolejną komendę */
    rx_index = 0;
    memset(rx_buffer, 0, RX_BUFFER_SIZE);
  }
}

void MasterApp_UART_ErrorCallback(void) {
  /* Zabezpieczenie przed zawieszeniem UART po odpięciu kabla (Hot-plug) */
  rx_index = 0;
  memset(rx_buffer, 0, RX_BUFFER_SIZE);

  const char *m =
      "UWAGA: Blad sprzetowy UART1. Zrestartowano nasluchiwanie!\r\n";
  HAL_UART_Transmit(&huart2, (uint8_t*) m, strlen(m), 100);  // NOLINT
}
