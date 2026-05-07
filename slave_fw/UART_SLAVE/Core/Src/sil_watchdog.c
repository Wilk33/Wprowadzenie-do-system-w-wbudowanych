// Copyright 2026 Tobiasz_Kandziora
#include "sil_watchdog.h"
#include "stm32l4xx_hal.h" /* Dla HAL_GetTick() */

// Przechowuje znacznik czasu ostatniego, poprawnego odebrania znaku życia (Ping)
static volatile uint32_t last_ping_time = 0;

// Wielomian dla sumy kontrolnej CRC-16 CCITT (używany w standardach telekomunikacyjnych i przemysłowych)
#define CRC_POLY 0x1021

/**
 * @brief Inicjuje Watchdoga aplikacyjnego.
 * @details Zapisuje czas startu układu, od którego zaczyna się odliczanie.
 */
void SIL_Watchdog_Init(void) {
  last_ping_time = HAL_GetTick();
}

/**
 * @brief "Karmienie" Watchdoga (resetowanie licznika czasu).
 * @details Należy wywołać tę funkcję każdorazowo po odebraniu prawidłowej
 *, autentycznej ramki od Mastera, aby zapobiec odcięciu układu.
 */
void SIL_Watchdog_Feed(void) {
  last_ping_time = HAL_GetTick();
}

/**
 * @brief Sprawdza, czy nastąpiło przekroczenie czasu oczekiwania na komunikację.
 * @retval 1 jeśli minął czas PING_TIMEOUT_MS (np. 1000 ms), 0 jeśli czas jest w normie.
 */
int SIL_Watchdog_IsExpired(void) {
  return ((HAL_GetTick() - last_ping_time) > PING_TIMEOUT_MS);
}

/**
 * @brief Oblicza 16-bitową sumę kontrolną (CRC) dla zadanego bufora danych.
 * @param data Wskaźnik na tablicę bajtów.
 * @param length Ilość bajtów do przeliczenia.
 * @retval Obliczona suma CRC-16.
 * @details Zabezpiecza przed fizycznym zniekształceniem bitów w kablu komunikacyjnym.
 */
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

/**
 * @brief Weryfikacja odebranej ramki życia (Heartbeat) z uwzględnieniem zasad SIL.
 * @param buffer Wskaźnik na zdekodowaną, 4-bajtową ramkę z UART.
 * @param last_seq Wskaźnik na pamięć o ostatnim numerze sekwencyjnym Mastera.
 * @param first_ping Wskaźnik na flagę pierwszego uruchomienia.
 * @retval 1 jeśli Ping jest poprawny (Watchdog nakarmiony), 0 jeśli odrzucony (błąd danych).
 */
int SIL_ProcessPingFrame(uint8_t *buffer, uint8_t *last_seq,
                         uint8_t *first_ping) {
  // 1. Sprawdzenie zgodności matematycznej (CRC) ramki
  uint16_t calculated_crc = SIL_CalculateCRC(buffer, 2);  // Liczymy dla "H" i numeru sekwencji
  uint16_t received_crc = (buffer[2] << 8) | buffer[3];  // Odtwarzamy CRC wysłane przez Mastera

  if (calculated_crc != received_crc)
    return 0; /* Złe CRC - odrzucamy ramkę (zakłócenia na linii) */

  // 2. Sprawdzenie, czy Master nie zawiesił się i nie wysyła starych danych
  uint8_t current_seq = buffer[1];
  if (*first_ping && (current_seq == *last_seq))
    return 0; /* Master zawieszony (utknął w pętli) - odrzucamy ramkę */

  // Zapisanie aktualnych poprawnych danych
  *last_seq = current_seq;
  *first_ping = 1;

  // Odświeżenie licznika czasu bezpieczeństwa
  SIL_Watchdog_Feed();
  return 1;
}
