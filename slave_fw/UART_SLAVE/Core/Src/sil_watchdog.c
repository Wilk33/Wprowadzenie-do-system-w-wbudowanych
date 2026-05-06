//Copyright 2026 Tobiasz_Kandziora
#include "sil_watchdog.h"
#include "stm32l4xx_hal.h" /* Dla HAL_GetTick() */

static volatile uint32_t last_ping_time = 0;
#define CRC_POLY 0x1021

void SIL_Watchdog_Init(void) {
  last_ping_time = HAL_GetTick();
}

void SIL_Watchdog_Feed(void) {
  last_ping_time = HAL_GetTick();
}

int SIL_Watchdog_IsExpired(void) {
  return ((HAL_GetTick() - last_ping_time) > PING_TIMEOUT_MS);
}

uint16_t SIL_CalculateCRC(uint8_t *data, uint8_t length) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < length; i++) {
    crc ^= (uint16_t) (data[i] << 8);
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x8000)
        crc = (crc << 1) ^ CRC_POLY;
      else
        crc = crc << 1;
      crc &= 0xFFFF;
    }
  }
  return crc;
}

/* Zwraca 1 jeśli Ping jest poprawny, 0 jeśli odrzucony */
int SIL_ProcessPingFrame(uint8_t *buffer, uint8_t *last_seq,
                         uint8_t *first_ping) {
  uint16_t calculated_crc = SIL_CalculateCRC(buffer, 2);
  uint16_t received_crc = (buffer[2] << 8) | buffer[3];

  if (calculated_crc != received_crc)
    return 0; /* Złe CRC */

  uint8_t current_seq = buffer[1];
  if (*first_ping && (current_seq == *last_seq))
    return 0; /* Master zawieszony */

  *last_seq = current_seq;
  *first_ping = 1;

  SIL_Watchdog_Feed();
  return 1;
}
