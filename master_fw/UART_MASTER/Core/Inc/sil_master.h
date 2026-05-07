// Copyright 2026 Tobiasz_Kandziora

// Ten moduł odpowiada za generowanie cyklicznego znaku życia (Heartbeat)
// oraz dostarcza algorytmy matematyczne sprawdzające integralność danych (CRC).

#ifndef MASTER_FW_UART_MASTER_CORE_INC_SIL_MASTER_H_
#define MASTER_FW_UART_MASTER_CORE_INC_SIL_MASTER_H_

#include <stdint.h>

/**
 * @brief Inicjalizacja modułu bezpieczeństwa Mastera.
 * @details Ustawia początkowy czas odliczania interwałów komunikacyjnych.
 */
void SIL_Master_Init(void);

/**
 * @brief Główna pętla wysyłania znaku życia (Heartbeat).
 * @details Należy wywoływać cyklicznie. Zapewnia ciągłość komunikacji ze Slave'm.
 */
void SIL_Master_Process(void);

/**
 * @brief Oblicza 16-bitową sumę kontrolną (CRC) dla wysyłanych danych.
 * @param data Wskaźnik na tablicę bajtów do przeliczenia.
 * @param length Ilość bajtów do przeliczenia.
 * @return Wyliczona wartość CRC-16.
 */
uint16_t SIL_CalculateCRC(uint8_t *data, uint16_t length);

#endif  // MASTER_FW_UART_MASTER_CORE_INC_SIL_MASTER_H_
