// Copyright 2026 Tobiasz_Kandziora
#include "master_app.h" // NOLINT
#include "sil_master.h" // NOLINT
#include "stm32l4xx_hal.h" // NOLINT
#include <stdio.h> // NOLINT
#include <string.h> // NOLINT
#include <stdlib.h> // NOLINT

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

// Maksymalny czas na przetworzenie żądania autoryzacji
#define AUTH_TIMEOUT_MS 5000
#define RX_BUFFER_SIZE 20

// Maszyna stanów aplikacji Mastera
typedef enum {
  MASTER_IDLE,                 // Stan oczekiwania na komendy
  MASTER_PROCESSING_AUTH,      // Przetwarzanie żądania autoryzacji
  MASTER_AUTHORIZATION_DONE  // Autoryzacja zakończona (przejście do sygnalizacji)
} MasterState_t;

static MasterState_t master_state = MASTER_IDLE;
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint8_t rx_index = 0;
static uint32_t auth_start_time = 0;
static char last_command = '0';
static uint8_t authorization_response = 0;

/* --- Funkcje prywatne --- */

/**
 * @brief Wysyła odpowiedź autoryzacyjną do Slave'a i terminala PC.
 * @param response Wartość odpowiedzi: 1 (Zgoda) lub 0 (Odmowa).
 */
static void SendAuthorizationResponse(uint8_t response) {
  char response_msg[10];
  snprintf(response_msg, sizeof(response_msg), "%d\r\n", response);

  // Komunikat debugowy na terminal PC (UART2)
  HAL_UART_Transmit(&huart2, (uint8_t*) "SENDING_AUTH:", 13, 100);  // NOLINT
  HAL_UART_Transmit(&huart2, (uint8_t*) response_msg, strlen(response_msg),
                    100);  // NOLINT

  // Właściwa komenda sterująca do urządzenia Slave (UART1)
  HAL_UART_Transmit(&huart1, (uint8_t*) response_msg, strlen(response_msg),
                    100);  // NOLINT
}

/**
 * @brief Przetwarza prawidłowo odebrane żądanie autoryzacji.
 * @param command Numer komendy/diody (1-8), o którą pyta Slave.
 * @details Weryfikuje, czy żądana komenda mieści się w dozwolonym zakresie,
 * po czym automatycznie wyzwala odpowiedź autoryzującą.
 */
static void ProcessAuthorizationRequest(int command) {
  if (command >= 1 && command <= 8) {
    last_command = command;
    authorization_response = 1;  // Zgoda na wykonanie
    master_state = MASTER_AUTHORIZATION_DONE;
    SendAuthorizationResponse(1);
  } else {
    authorization_response = 0;  // Odmowa - komenda poza zakresem
    master_state = MASTER_AUTHORIZATION_DONE;
    SendAuthorizationResponse(0);
  }
}

/**
 * @brief Zabezpieczenie przed zablokowaniem systemu w stanie autoryzacji.
 * @details Jeśli przetwarzanie zapytania trwa zbyt długo, Master resetuje
 * swój bufor i wraca do stanu IDLE.
 */
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

/**
 * @brief Główna inicjalizacja aplikacji Mastera.
 */
void MasterApp_Init(void) {
  SIL_Master_Init();
  // Zapalenie wbudowanej diody LD3 jako wskaźnika gotowości
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
  master_state = MASTER_IDLE;
  rx_index = 0;
  memset(rx_buffer, 0, RX_BUFFER_SIZE);
}

/**
 * @brief Główna pętla wykonawcza Mastera (wywoływana w while(1)).
 */
void MasterApp_Process(void) {
  /* 1. Utrzymanie znaku życia dla Slave'a (Heartbeat) */
  SIL_Master_Process();

  /* 2. Sprawdzenie timeoutu zawieszonej autoryzacji */
  HandleAuthorizationTimeout();

  /* 3. Sygnalizacja wizualna po pomyślnej autoryzacji */
  if (master_state == MASTER_AUTHORIZATION_DONE) {
    // Mignięcie wbudowaną diodą LD3 na znak wysłania komendy
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);

    master_state = MASTER_IDLE;  // Powrót do nasłuchiwania
  }

  HAL_Delay(10); /* Lekki oddech dla procesora w pętli */
}

/**
 * @brief Przerwanie odbioru danych od Slave'a (UART1).
 * @param rx_byte Pojedynczy odebrany bajt.
 * @details Funkcja odkłada znaki do bufora aż do napotkania znaku końca linii (\n),
 * a następnie parsuje ciąg znaków, oddziela wartość CRC, przelicza ją
 * i decyduje o dopuszczeniu zapytania.
 */
void MasterApp_UART1_RxCallback(uint8_t rx_byte) {
  if (rx_index < RX_BUFFER_SIZE - 1) {
    rx_buffer[rx_index++] = rx_byte;
  }

  /* Wykrycie końca przesyłanej linii (komendy) */
  if (rx_byte == '\n') {
    /* Czyszczenie białych znaków (\r, \n) na końcu bufora */
    for (int i = 0; i < rx_index; i++) {
      if (rx_buffer[i] == '\r' || rx_buffer[i] == '\n') {
        rx_buffer[i] = '\0';
        break;
      }
    }

    /* Oczekiwany format: "Aut_req:<numer_komendy>,<suma_crc_HEX>" */
    if (strncmp((char*) rx_buffer, "Aut_req:", 8) == 0) {  // NOLINT
      int command = 0;
      uint16_t received_crc = 0;

      char *ptr = (char*) &rx_buffer[8];  // NOLINT
      command = atoi(ptr);

      // Przesuń wskaźnik do znaku przecinka, za którym znajduje się CRC
      while (*ptr && *ptr != ',')
        ptr++;
      if (*ptr == ',')
        ptr++;

      // Zamiana ciągu szesnastkowego z powrotem na liczbę
      received_crc = (uint16_t) strtol(ptr, NULL, 16);

      // Walidacja autentyczności komendy
      if (command > 0 && command <= 8) {
        uint8_t cmd_byte = (uint8_t) command;
        // Przeliczenie CRC po stronie Mastera dla tej samej komendy
        uint16_t calculated_crc = SIL_CalculateCRC(&cmd_byte, 1);

        // Ochrona integralności - upewnienie się, że zakłócenia nie zmieniły bajtu
        if (calculated_crc == received_crc) {
          HAL_UART_Transmit(&huart2, (uint8_t*) "CRC_OK - PROCESSING\r\n", 21,
                            100);
          ProcessAuthorizationRequest(command);  // NOLINT
        } else {
          // Odrzucenie komendy w przypadku uszkodzenia podczas transmisji
          HAL_UART_Transmit(&huart2, (uint8_t*) "CRC_FAIL\r\n", 10, 100);  // NOLINT
          SendAuthorizationResponse(0);
        }
      }
    } else {
      HAL_UART_Transmit(&huart2, (uint8_t*) "FMT_ERR\r\n", 9, 100);  // NOLINT
    }

    /* Reset bufora na kolejną komendę, by zapobiec wyciekom starych danych */
    rx_index = 0;
    memset(rx_buffer, 0, RX_BUFFER_SIZE);
  }
}

/**
 * @brief Przerwanie błędu sprzętowego UART (np. odłączenie kabla, Overrun).
 * @details Niezwykle ważny mechanizm dla przemysłu (Hot-plug). Zabezpiecza
 * układ przed wejściem w twardy, nieodwracalny błąd uC (HardFault) w razie
 * fizycznego rozpięcia magistrali, czyszcząc bufor na nowo.
 */
void MasterApp_UART_ErrorCallback(void) {
  rx_index = 0;
  memset(rx_buffer, 0, RX_BUFFER_SIZE);

  const char *m =
      "UWAGA: Blad sprzetowy UART1. Zrestartowano nasluchiwanie!\r\n";
  HAL_UART_Transmit(&huart2, (uint8_t*) m, strlen(m), 100);  // NOLINT
}
