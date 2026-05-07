// Copyright 2026 Tobiasz_Kandziora

// Ten plik zawiera główny interfejs aplikacji (mózg Mastera).
// Łączy ze sobą moduł bezpieczeństwa (SIL), maszynę stanów oraz przerwania UART.

#ifndef MASTER_FW_UART_MASTER_CORE_INC_MASTER_APP_H_
#define MASTER_FW_UART_MASTER_CORE_INC_MASTER_APP_H_

#include <stdint.h>
#include "main.h" // NOLINT

/**
 * @brief Główna inicjalizacja aplikacji Mastera.
 * @details Odpala podmoduły, resetuje bufory i ustala stan początkowy.
 */
void MasterApp_Init(void);

/**
 * @brief Główna pętla wykonawcza Mastera.
 * @details Odpowiada za zarządzanie autoryzacją i nadzorowanie timeoutów.
 * Należy umieścić wewnątrz głównej pętli while(1).
 */
void MasterApp_Process(void);

/**
 * @brief Przerwanie odbioru danych od Slave'a (UART1).
 * @param rx_byte Pojedynczy odebrany bajt.
 */
void MasterApp_UART1_RxCallback(uint8_t rx_byte);

/**
 * @brief Przerwanie błędu sprzętowego UART (np. odłączenie kabla, Overrun).
 * @details Zabezpiecza przed trwałym zablokowaniem sprzętu. Wywoływana z ErrorCallback.
 */
void MasterApp_UART_ErrorCallback(void);

#endif  // MASTER_FW_UART_MASTER_CORE_INC_MASTER_APP_H_
