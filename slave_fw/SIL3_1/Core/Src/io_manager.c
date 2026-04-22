// Copyright 2026 Gruby
#include "io_manager.h"

// Mapowanie Twoich pinów diod (przykładowe)
static GPIO_TypeDef *LED_PORTS[] = {GPIOA, GPIOA, GPIOA, GPIOA,
                                    GPIOB, GPIOB, GPIOB, GPIOB};
static uint16_t LED_PINS[] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3,
                              GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3};

void IO_SetLeds(uint8_t count) {
  for (int i = 0; i < 8; i++) {
    GPIO_PinState state = (i < count) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(LED_PORTS[i], LED_PINS[i], state);
  }
}

void IO_AllOff(void) { IO_SetLeds(0); }

void IO_RunSelfTest(void) {
  for (int i = 1; i <= 8; i++) {
    IO_SetLeds(i);
    HAL_Delay(50); // Tylko podczas testu startowego dopuszczalne
  }
  IO_AllOff();
}

void IO_Init(void) {
  // Tutaj może być pusto, ale funkcja MUSI fizycznie istnieć
  // Możesz tu np. zgasić wszystkie diody na start
  IO_AllOff();
}
