// Copyright 2026 Tobiasz_Kandziora

// --- Załączniki ---
#include "slave_app.h"
#include <stdio.h>
#include <string.h>

#include "led_driver.h"
#include "sil_watchdog.h"
#include "stm32l4xx_hal.h"

// Zewnętrzne uchwyty do sprzętowego UART wygenerowane przez uC
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

// --- Zmienne i definicje ---
#define AUTH_TIMEOUT_MS    5000  // Maksymalny czas oczekiwania na autoryzację od Mastera
#define SAFETY_TIMEOUT_MS  3000  // Czas trwania bezpiecznego rozruchu (Safe Boot)

// Stany maszyny stanów układu wykonawczego (Slave)
typedef enum {
  STATE_IDLE,                 // Oczekiwanie na komendy z PC
  STATE_WAITING_MASTER_AUTH,  // Wysłano komendę do Mastera, oczekiwanie na zgodę
  STATE_COMMAND_AUTHORIZED,   // Master zatwierdził komendę, wykonano akcję
  STATE_ERROR,                // Błąd autoryzacji (Master odrzucił)
  STATE_SAFETY,               // Tryb Safe Boot (blokada po starcie)
  STATE_SAFETY_PERMANENT  // Trwały błąd bezpieczeństwa (brak komunikacji z Masterem)
} SlaveState_t;

// Zmienne zarządzające stanem i czasem
static volatile SlaveState_t slave_state = STATE_SAFETY;  // Startujemy w trybie bezpiecznym
static uint32_t auth_timeout = 0;
static uint32_t safety_entry_time = 0;
static uint8_t leds_to_set = 0;

// Zmienne do odbioru i parsowania ramki Ping (Heartbeat)
static volatile uint8_t ping_rx_buffer[4];
static volatile uint8_t ping_rx_index = 0;
static volatile uint8_t last_ping_seq = 0;
static volatile uint8_t first_ping_received = 0;

/**
 * @brief Funkcja wysyłania wiadomości przez UART2 (do PC).
 * @param msg Wskaźnik na łańcuch znaków do wysłania.
 */
static void SendMessage(const char *msg) {
  HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), 100);
}

/**
 * @brief Żądanie autoryzacji komendy u Mastera.
 * @param command Numer diody (komendy) do autoryzacji.
 * @details Oblicza sumę kontrolną CRC dla komendy, formuje bezpieczną ramkę
 * i wysyła ją przez UART1. Następnie przełącza stan układu na oczekiwanie.
 */
static void SendAuthorizationRequest(uint8_t command) {
  uint16_t crc = SIL_CalculateCRC(&command, 1);
  char buffer[24];

  // Bezpieczne formatowanie bufora
  uint8_t len = snprintf(buffer, sizeof(buffer), "Aut_req:%d,%04X\r\n", command,
                         crc);

  HAL_UART_Transmit(&huart1, (uint8_t*) buffer, len, 100);

  slave_state = STATE_WAITING_MASTER_AUTH;
  auth_timeout = HAL_GetTick();  // Zapisanie czasu startu oczekiwania
}

/**
 * @brief Inicjalizacja aplikacji Slave.
 * @details Ustawia stan początkowy na SAFE BOOT i uruchamia Watchdoga SIL.
 */
void SlaveApp_Init(void) {
  SendMessage("STM32 Slave! Safe Boot: Czekam 3s na stabilizacje...\r\n");
  slave_state = STATE_SAFETY;
  safety_entry_time = HAL_GetTick();
  SIL_Watchdog_Init();
}

/**
 * @brief Główna pętla maszyny stanów aplikacji Slave'a.
 * @details Zarządza trybem Safe Boot, nadzoruje Watchdoga (SIL3 Heartbeat)
 * oraz kontroluje timeouty. Wywoływana cyklicznie w pętli main().
 */
void SlaveApp_Process(void) {
  /* 1. Safe Boot Timeout - odliczanie 3 sekund bezpiecznego startu */
  if (slave_state == STATE_SAFETY) {
    if ((HAL_GetTick() - safety_entry_time) >= SAFETY_TIMEOUT_MS) {
      slave_state = STATE_IDLE;
      SIL_Watchdog_Feed(); /* Odświeżenie Watchdoga przed startem, żeby nie wyrzucił błędu */
      SendMessage("SAFE BOOT ZAKONCZONY -> Gotowy do pracy (IDLE)\r\n");
    }
    return; /* Blokada przetwarzania reszty logiki, dopóki trwa Safe Boot */
  }

  /* 2. SIL Watchdog - sprawdzanie ciągłości komunikacji */
  if (slave_state != STATE_SAFETY_PERMANENT) {
    if (SIL_Watchdog_IsExpired()) {
      slave_state = STATE_SAFETY_PERMANENT;
      LED_Set(0); /* Odcięcie wyjść fizycznych w razie błędu */
      SendMessage("SIL ALERT: Timeout Pingu! SAFETY PERMANENT aktywny.\r\n");
    }
  }

  /* 3. Auth Timeout - sprawdzanie, czy Master odpowiedział na czas */
  if (slave_state == STATE_WAITING_MASTER_AUTH) {
    if ((HAL_GetTick() - auth_timeout) > AUTH_TIMEOUT_MS) {
      SendMessage("BLAD: Timeout autoryzacji!\r\n");
      slave_state = STATE_IDLE; /* Powrót do nasłuchiwania w przypadku braku odpowiedzi */
    }
  }

  /* 4. Aktualizacja LED / Self-test - krokowe zapalanie diod w trybie testu */
  LED_SelfTest_Update();
  if (LED_IsTesting() == 0 && slave_state == STATE_IDLE
      && slave_state != STATE_COMMAND_AUTHORIZED) {
    /* Opcjonalny powrót z testu */
  }
}

/**
 * @brief Funkcja obsługi UART1 - komunikacja z Masterem (przerwania).
 * @param rx_byte Pojedynczy odebrany bajt z przerwania sprzętowego.
 * @details Parsuje odpowiedzi na autoryzację ('0' lub '1') oraz składa
 * 4-bajtową ramkę Pingu zaczynającą się od litery 'H'.
 */
void SlaveApp_UART1_RxCallback(uint8_t rx_byte) {
  if (ping_rx_index == 0) {
    // Nasłuchiwanie na początek ramki Pingu
    if (rx_byte == 'H') {
      ping_rx_buffer[0] = rx_byte;
      ping_rx_index = 1;
    }
    // Odpowiedź od Mastera na autoryzację (1 = Zgoda, 0 = Odmowa)
    else if (rx_byte == '0' || rx_byte == '1') {
      if (slave_state == STATE_WAITING_MASTER_AUTH) {
        if (rx_byte == '1') {
          LED_Set(leds_to_set);  // Wykonanie autoryzowanego polecenia
          SendMessage("Operacja wykonana!\r\n");
          slave_state = STATE_COMMAND_AUTHORIZED;
        } else {
          SendMessage("BLAD: Brak autoryzacji!\r\n");
          slave_state = STATE_ERROR;
        }
      }
    }
  } else {
    // Zbieranie pozostałych bajtów ramki Pingu (sekwencja + 2 bajty CRC)
    ping_rx_buffer[ping_rx_index++] = rx_byte;

    // Jeśli zebrano całą 4-bajtową ramkę, następuje walidacja
    if (ping_rx_index == 4) {
      SIL_ProcessPingFrame((uint8_t*) ping_rx_buffer, (uint8_t*) &last_ping_seq,
                           (uint8_t*) &first_ping_received);
      ping_rx_index = 0;  // Reset bufora dla kolejnej ramki
    }
  }
}

/**
 * @brief Funkcja obsługi UART2 - komunikacja z PC użytkownika (przerwania).
 * @param rx_byte Pojedynczy odebrany bajt.
 * @details Przyjmuje komendy od użytkownika, nakłada blokady w stanach
 * awaryjnych oraz uruchamia procedury takie jak Self-Test czy Reset.
 */
void SlaveApp_UART2_RxCallback(uint8_t rx_byte) {
  // Jeśli jesteśmy w trybie safety, żadne komendy nie działają.
  if (slave_state == STATE_SAFETY) {
    SendMessage("BLAD: Trwa SAFE BOOT!\r\n");
    return;
  }

  // Obsługa permanentnego błędu - przy wyjściu ('p') obowiązkowy reset watchdoga.
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

  // Jeśli wyślemy UART-em 'r' lub 'R', wysyła komunikat i robi reset programowy.
  if (rx_byte == 'r' || rx_byte == 'R') {
    SendMessage("SOFTWARE RESET\r\n");
    // Oczekiwanie na faktyczne opuszczenie bufora TX przed restartem systemu
    while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == RESET) {
    }
    NVIC_SystemReset();
    return;
  }

  // Jeśli wyślemy 't' lub 'T', wykonuje selftest - zapala ledy od 1 do 8.
  if (rx_byte == 't' || rx_byte == 'T') {
    SendMessage("SELFTEST: Start\r\n");
    LED_SelfTest_Start();
    return;
  }

  // Wysłanie cyfry 1-8 inicjuje autoryzację zapalenia danego LED-a u Mastera.
  // Jeśli jesteśmy w trybie testu, to zostaje on najpierw przerwany.
  if (rx_byte >= '1' && rx_byte <= '8') {
    if (LED_IsTesting())
      LED_SelfTest_Abort();

    leds_to_set = rx_byte - '0';  // Konwersja znaku ASCII na wartość liczbową
    SendAuthorizationRequest(leds_to_set);
  }
}
