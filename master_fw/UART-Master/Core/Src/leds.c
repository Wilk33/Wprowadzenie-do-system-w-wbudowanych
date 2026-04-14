/* leds.c - LED/Buttons handling
 */

#include "leds.h"
#include "stm32l4xx_hal.h"

/* Forward declaration of uart API (implemented in uart.c) */
extern void UART_SendLedValue(uint8_t value);
extern uint8_t UART_IsConnected(void);

static volatile uint8_t s_led_number = 0;
static volatile uint32_t s_last_button_press_time = 0;
static const uint32_t s_debounce_delay = 150;

void Led_Init(void) {
    s_led_number = 0;
    Update_LEDs(0);
}

void Update_LEDs(uint8_t count) {
    HAL_GPIO_WritePin(Led_1_GPIO_Port, Led_1_Pin, (count >= 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_2_GPIO_Port, Led_2_Pin, (count >= 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_3_GPIO_Port, Led_3_Pin, (count >= 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_4_GPIO_Port, Led_4_Pin, (count >= 4) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_5_GPIO_Port, Led_5_Pin, (count >= 5) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_6_GPIO_Port, Led_6_Pin, (count >= 6) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_7_GPIO_Port, Led_7_Pin, (count >= 7) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_8_GPIO_Port, Led_8_Pin, (count >= 8) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t Led_GetNumber(void) {
    return (uint8_t)s_led_number;
}

void Led_OnDisconnect(void) {
    s_led_number = 0;
    Update_LEDs(0);
    HAL_GPIO_WritePin(Led_Signal_GPIO_Port, Led_Signal_Pin, GPIO_PIN_SET);
}

/* EXTI callback for buttons — exported as the standard HAL callback name so it will be used
 * Only button logic lives here; UART handling is delegated to uart module. */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    uint32_t now = HAL_GetTick();
    if ((now - s_last_button_press_time) < s_debounce_delay) return;
    s_last_button_press_time = now;

    if (UART_IsConnected()) {
        if (GPIO_Pin == Button_1_Pin) {
            s_led_number++;
            if (s_led_number > 8) s_led_number = 0;
            Update_LEDs(s_led_number);
        }
        if (GPIO_Pin == Button_2_Pin) {
            UART_SendLedValue((uint8_t)s_led_number);
        }
    }
    else {
        if (GPIO_Pin == Button_Signal_Pin) {
            HAL_GPIO_WritePin(Led_Signal_GPIO_Port, Led_Signal_Pin, GPIO_PIN_RESET);
            /* inform UART layer that connection is OK */
            extern void UART_SetConnectionOk(void);
            UART_SetConnectionOk();
        }
    }
}
