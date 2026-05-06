//Copyright 2026 Tobiasz_Kandziora
#include "led_driver.h"
#include "main.h" /* Dla definicji pinów wygenerowanych przez CubeMX */

typedef struct {
  int current_led;
  uint32_t last_time;
  int is_testing;
} SelfTest_State;

static SelfTest_State selftest = { 0, 0, 0 };

/* Tablice pinów dla łatwiejszej iteracji */
static const GPIO_TypeDef *ports[] = { Led_1_GPIO_Port, Led_2_GPIO_Port,
    Led_3_GPIO_Port, Led_4_GPIO_Port, Led_5_GPIO_Port, Led_6_GPIO_Port,
    Led_7_GPIO_Port, Led_8_GPIO_Port };
static const uint16_t pins[] = { Led_1_Pin, Led_2_Pin, Led_3_Pin, Led_4_Pin,
    Led_5_Pin, Led_6_Pin, Led_7_Pin, Led_8_Pin };

void LED_Set(uint8_t status) {
  for (int i = 0; i < 8; i++) {
    HAL_GPIO_WritePin((GPIO_TypeDef*) ports[i], pins[i],
                      (status == (i + 1)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }
}

void LED_SelfTest_Start(void) {
  selftest.current_led = 0;
  selftest.is_testing = 1;
  selftest.last_time = HAL_GetTick();
}

void LED_SelfTest_Abort(void) {
  selftest.is_testing = 0;
  LED_Set(0);
}

int LED_IsTesting(void) {
  return selftest.is_testing;
}

void LED_SelfTest_Update(void) {
  if (!selftest.is_testing)
    return;

  uint32_t now = HAL_GetTick();
  if ((now - selftest.last_time) < 500)
    return;

  if (selftest.current_led < 8) {
    if (selftest.current_led > 0) {
      HAL_GPIO_WritePin((GPIO_TypeDef*) ports[selftest.current_led - 1],
                        pins[selftest.current_led - 1], GPIO_PIN_RESET);
    }
    HAL_GPIO_WritePin((GPIO_TypeDef*) ports[selftest.current_led],
                      pins[selftest.current_led], GPIO_PIN_SET);
    selftest.current_led++;
    selftest.last_time = now;
  } else {
    HAL_GPIO_WritePin((GPIO_TypeDef*) ports[7], pins[7], GPIO_PIN_RESET);
    selftest.is_testing = 0;
  }
}
