/*
 * app.c
 * Application stub - moved logic to uart and leds modules
 */

#include "app.h"

void App_Init(void) {
    /* stub: application logic moved to uart/leds modules */
}

void App_Start(TIM_HandleTypeDef *htim, UART_HandleTypeDef *huart) {
    /* stub: use UART_Start in uart module instead */
    (void)htim;
    (void)huart;
}