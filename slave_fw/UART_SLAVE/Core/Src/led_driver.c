// Copyright 2026 Tobiasz_Kandziora
#include "led_driver.h"
#include "main.h" /* Dla definicji pinów wygenerowanych przez CubeMX */

// Struktura przechowująca stan procedury diagnostycznej (Self-Test)
typedef struct {
  int current_led;      // Aktualnie zapalona dioda (0-7)
  uint32_t last_time;   // Znacznik czasu ostatniego przełączenia diody
  int is_testing;  // Flaga informująca, czy test jest w toku (1 = tak, 0 = nie)
} SelfTest_State;

static SelfTest_State selftest = { 0, 0, 0 };

/* Tablice pinów dla łatwiejszej iteracji w pętlach for.
 * Pozwala to uniknąć pisania 8 oddzielnych instrukcji warunkowych if/else. */
static const GPIO_TypeDef *ports[] = { Led_1_GPIO_Port, Led_2_GPIO_Port,
Led_3_GPIO_Port, Led_4_GPIO_Port, Led_5_GPIO_Port, Led_6_GPIO_Port,
Led_7_GPIO_Port, Led_8_GPIO_Port };
static const uint16_t pins[] = { Led_1_Pin, Led_2_Pin, Led_3_Pin, Led_4_Pin,
Led_5_Pin, Led_6_Pin, Led_7_Pin, Led_8_Pin };

/**
 * @brief Ustawia stan fizycznych wyjść mikrokontrolera (diod LED).
 * @param status Numer diody do zapalenia (1-8). Wartość 0 gasi wszystkie diody.
 */
void LED_Set(uint8_t status) {
  for (int i = 0; i < 8; i++) {
    // Jeśli status odpowiada aktualnemu indeksowi (np. status 1 -> indeks 0), zapal diodę, resztę zgaś.
    HAL_GPIO_WritePin((GPIO_TypeDef*) ports[i], pins[i],
                      (status == (i + 1)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }
}

/**
 * @brief Rozpoczyna sekwencyjny test wszystkich diod (Self-Test).
 */
void LED_SelfTest_Start(void) {
  selftest.current_led = 0;
  selftest.is_testing = 1;
  selftest.last_time = HAL_GetTick();
}

/**
 * @brief Przymusowo przerywa trwający Self-Test i gasi wszystkie diody.
 */
void LED_SelfTest_Abort(void) {
  selftest.is_testing = 0;
  LED_Set(0);  // Przekazanie 0 wymusza zgaszenie całego paska LED
}

/**
 * @brief Sprawdza, czy procedura testowa jest obecnie wykonywana.
 * @retval 1 jeśli test trwa, 0 jeśli układ jest w trybie normalnym.
 */
int LED_IsTesting(void) {
  return selftest.is_testing;
}

/**
 * @brief Aktualizuje stan Self-Testu (maszyna stanów testu).
 * @details Funkcja powinna być wywoływana cyklicznie. Zmienia świecącą diodę
 * na kolejną co określony czas (500 ms). Po dojściu do końca, wyłącza test.
 */
void LED_SelfTest_Update(void) {
  if (!selftest.is_testing)
    return;  // Jeśli test nie jest aktywny, przerwij wykonywanie funkcji

  uint32_t now = HAL_GetTick();
  if ((now - selftest.last_time) < 500)
    return;  // Oczekujemy na upłynięcie 500 ms od ostatniej zmiany

  if (selftest.current_led < 8) {
    // Zgaś poprzednią diodę (jeśli to nie jest pierwszy krok)
    if (selftest.current_led > 0) {
      HAL_GPIO_WritePin((GPIO_TypeDef*) ports[selftest.current_led - 1],
                        pins[selftest.current_led - 1], GPIO_PIN_RESET);
    }
    // Zapal obecną diodę
    HAL_GPIO_WritePin((GPIO_TypeDef*) ports[selftest.current_led],
                      pins[selftest.current_led], GPIO_PIN_SET);

    selftest.current_led++;
    selftest.last_time = now;
  } else {
    // Koniec testu - zgaś ostatnią (ósmą) diodę i wyłącz flagę testowania
    HAL_GPIO_WritePin((GPIO_TypeDef*) ports[7], pins[7], GPIO_PIN_RESET);
    selftest.is_testing = 0;
  }
}
