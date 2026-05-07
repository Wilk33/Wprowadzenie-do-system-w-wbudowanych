// Copyright 2026 Tobiasz_Kandziora
#include "sil_master.h" // NOLINT
#include "stm32l4xx_hal.h" //NOLINT

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

// Wielomian dla sumy kontrolnej CRC-16 CCITT
#define CRC_POLY 0x1021
// Częstotliwość wysyłania znaku życia (Heartbeat) - 500 ms
#define PING_INTERVAL_MS 500

static uint32_t last_ping_sent_time = 0;
static uint8_t ping_sequence = 0;

/**
 * @brief Inicjalizacja modułu bezpieczeństwa Mastera.
 * @details Zapisuje początkowy czas (tick) mikrokontrolera do poprawnego
 * rozpoczęcia odliczania interwałów Pingu.
 */
void SIL_Master_Init(void) {
  last_ping_sent_time = HAL_GetTick();
}

/**
 * @brief Główna pętla wysyłania znaku życia (Heartbeat).
 * @details Cyklicznie generuje ramkę potwierdzającą poprawną pracę Mastera.
 * Zgodnie z założeniami SIL, ramka składa się ze stałego nagłówka ('H'),
 * zmieniającego się numeru sekwencji oraz sumy kontrolnej CRC zabezpieczającej
 * przed zniekształceniami na linii komunikacyjnej.
 */
void SIL_Master_Process(void) {
  /* Cykliczne wysyłanie Pingu co zdefiniowany interwał czasowy */
  if ((HAL_GetTick() - last_ping_sent_time) >= PING_INTERVAL_MS) {
    uint8_t ping_frame[4];

    ping_frame[0] = 'H';             // Znak synchronizacji ramki
    ping_frame[1] = ping_sequence;   // Zmieniający się numer sekwencji

    // Obliczenie sumy kontrolnej dla nagłówka i numeru sekwencji
    uint16_t crc = SIL_CalculateCRC(ping_frame, 2);

    // Rozbicie 16-bitowego CRC na dwa oddzielne bajty (Big-Endian)
    ping_frame[2] = (uint8_t) ((crc >> 8) & 0xFF);
    ping_frame[3] = (uint8_t) (crc & 0xFF);

    // Wysłanie zmontowanej 4-bajtowej ramki przez port UART połączony ze Slave'm
    HAL_UART_Transmit(&huart1, ping_frame, 4, 100);

    // Inkrementacja sekwencji do wykorzystania w kolejnym cyklu
    ping_sequence++;
    last_ping_sent_time = HAL_GetTick();
  }
}

/**
 * @brief Oblicza 16-bitową sumę kontrolną (CRC) dla wysyłanych danych.
 * @param data Wskaźnik na tablicę bajtów do przeliczenia.
 * @param length Ilość bajtów do przeliczenia.
 * @retval Obliczona suma CRC-16.
 * @details Algorytm weryfikujący integralność danych, konieczny do
 * spełnienia rygorów bezpieczeństwa transmisji.
 */
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
