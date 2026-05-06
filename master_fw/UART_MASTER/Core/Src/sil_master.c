// Copyright 2026 Tobiasz_Kandziora
#include "sil_master.h" // NOLINT
#include "stm32l4xx_hal.h" //NOLINT

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

#define CRC_POLY 0x1021
#define PING_INTERVAL_MS 500

static uint32_t last_ping_sent_time = 0;
static uint8_t ping_sequence = 0;

void SIL_Master_Init(void) {
  last_ping_sent_time = HAL_GetTick();
}

void SIL_Master_Process(void) {
  /* Cykliczne wysyłanie Pingu (Heartbeat) */
  if ((HAL_GetTick() - last_ping_sent_time) >= PING_INTERVAL_MS) {
    uint8_t ping_frame[4];

    ping_frame[0] = 'H';
    ping_frame[1] = ping_sequence;

    uint16_t crc = SIL_CalculateCRC(ping_frame, 2);

    ping_frame[2] = (uint8_t) ((crc >> 8) & 0xFF);
    ping_frame[3] = (uint8_t) (crc & 0xFF);

    HAL_UART_Transmit(&huart1, ping_frame, 4, 100);

    ping_sequence++;
    last_ping_sent_time = HAL_GetTick();
  }
}

uint16_t SIL_CalculateCRC(uint8_t *data, uint16_t length) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < length; i++) {
    crc ^= (data[i] << 8);
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
