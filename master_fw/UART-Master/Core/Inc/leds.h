/* leds.h
 * LED control module
 */
#ifndef __LEDS_H
#define __LEDS_H

#include <stdint.h>
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

void Led_Init(void);
void Update_LEDs(uint8_t count);
uint8_t Led_GetNumber(void);
void Led_OnDisconnect(void);

#ifdef __cplusplus
}
#endif

#endif /* __LEDS_H */
