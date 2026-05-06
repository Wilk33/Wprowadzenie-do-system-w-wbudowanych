// Copyright 2026 Tobiasz_Kandziora
#include "slave_app.h"
#include <stdio.h>
#include <string.h>

#include "led_driver.h"
#include "sil_watchdog.h"
#include "stm32l4xx_hal.h"


extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

#define AUTH_TIMEOUT_MS    5000
#define SAFETY_TIMEOUT_MS  3000

typedef enum {
  STATE_IDLE,
  STATE_WAITING_MASTER_AUTH,
  STATE_COMMAND_AUTHORIZED,
  STATE_ERROR,
  STATE_SAFETY,
  STATE_SAFETY_PERMANENT
} SlaveState_t;

static volatile SlaveState_t slave_state = STATE_SAFETY;
static uint32_t auth_timeout = 0;
static uint32_t safety_entry_time = 0;
static uint8_t leds_to_set = 0;

static volatile uint8_t ping_rx_buffer[4];
static volatile uint8_t ping_rx_index = 0;
static volatile uint8_t last_ping_seq = 0;
static volatile uint8_t first_ping_received = 0;

static void SendMessage(const char *msg) {
  HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), 100);
}

static void SendAuthorizationRequest(uint8_t command) {
  uint16_t crc = SIL_CalculateCRC(&command, 1);
  char buffer[24];
  uint8_t len = snprintf(buffer, sizeof(buffer), "Aut_req:%d,%04X\r\n", command, crc);
  HAL_UART_Transmit(&huart1, (uint8_t*) buffer, len, 100);
  slave_state = STATE_WAITING_MASTER_AUTH;
  auth_timeout = HAL_GetTick();
}

void SlaveApp_Init(void) {
  SendMessage("STM32 Slave! Safe Boot: Czekam 3s na stabilizacje...\r\n");
  slave_state = STATE_SAFETY;
  safety_entry_time = HAL_GetTick();
  SIL_Watchdog_Init();
}

void SlaveApp_Process(void) {
  /* 1. Safe Boot Timeout */
  if (slave_state == STATE_SAFETY) {
    if ((HAL_GetTick() - safety_entry_time) >= SAFETY_TIMEOUT_MS) {
      slave_state = STATE_IDLE;
      SIL_Watchdog_Feed(); /* Odświeżenie przed startem */
      SendMessage("SAFE BOOT ZAKONCZONY -> Gotowy do pracy (IDLE)\r\n");
    }
    return;
  }

  /* 2. SIL Watchdog */
  if (slave_state != STATE_SAFETY_PERMANENT) {
    if (SIL_Watchdog_IsExpired()) {
      slave_state = STATE_SAFETY_PERMANENT;
      LED_Set(0);
      SendMessage("SIL ALERT: Timeout Pingu! SAFETY PERMANENT aktywny.\r\n");
    }
  }

  /* 3. Auth Timeout */
  if (slave_state == STATE_WAITING_MASTER_AUTH) {
    if ((HAL_GetTick() - auth_timeout) > AUTH_TIMEOUT_MS) {
      SendMessage("BLAD: Timeout autoryzacji!\r\n");
      slave_state = STATE_IDLE;
    }
  }

  /* 4. Aktualizacja LED / Self-test */
  LED_SelfTest_Update();
  if (LED_IsTesting() == 0 && slave_state == STATE_IDLE
      && slave_state != STATE_COMMAND_AUTHORIZED) {
    /* Opcjonalny powrót z testu */
  }
}

void SlaveApp_UART1_RxCallback(uint8_t rx_byte) {
  if (ping_rx_index == 0) {
    if (rx_byte == 'H') {
      ping_rx_buffer[0] = rx_byte;
      ping_rx_index = 1;
    } else if (rx_byte == '0' || rx_byte == '1') {
      if (slave_state == STATE_WAITING_MASTER_AUTH) {
        if (rx_byte == '1') {
          LED_Set(leds_to_set);
          SendMessage("Operacja wykonana!\r\n");
          slave_state = STATE_COMMAND_AUTHORIZED;
        } else {
          SendMessage("BLAD: Brak autoryzacji!\r\n");
          slave_state = STATE_ERROR;
        }
      }
    }
  } else {
    ping_rx_buffer[ping_rx_index++] = rx_byte;
    if (ping_rx_index == 4) {
      SIL_ProcessPingFrame((uint8_t*) ping_rx_buffer, (uint8_t*) &last_ping_seq,
                           (uint8_t*) &first_ping_received);
      ping_rx_index = 0;
    }
  }
}

void SlaveApp_UART2_RxCallback(uint8_t rx_byte) {
  if (slave_state == STATE_SAFETY) {
    SendMessage("BLAD: Trwa SAFE BOOT!\r\n");
    return;
  }

  if (slave_state == STATE_SAFETY_PERMANENT) {
    if (rx_byte == 'p' || rx_byte == 'P') {
      slave_state = STATE_IDLE;
      SIL_Watchdog_Feed();
      SendMessage("SAFETY PERMANENT -> IDLE\r\n");
    } else {
      SendMessage("BLAD: SAFETY PERMANENT aktywny!\r\n");
    }
    return;
  }

  /* Software Reset - dodany z powrotem! */
  if (rx_byte == 'r' || rx_byte == 'R') {
    SendMessage("SOFTWARE RESET\r\n");
    while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == RESET) {
    }
    NVIC_SystemReset();
    return;
  }

  if (rx_byte == 't' || rx_byte == 'T') {
    SendMessage("SELFTEST: Start\r\n");
    LED_SelfTest_Start();
    return;
  }

  if (rx_byte >= '1' && rx_byte <= '8') {
    if (LED_IsTesting())
      LED_SelfTest_Abort();
    leds_to_set = rx_byte - '0';
    SendAuthorizationRequest(leds_to_set);
  }
}
