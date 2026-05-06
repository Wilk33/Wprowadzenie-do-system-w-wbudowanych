//Copyright 2026 Tobiasz_Kandziora
#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <stdint.h>

void LED_Set(uint8_t status);
void LED_SelfTest_Start(void);
void LED_SelfTest_Update(void);
void LED_SelfTest_Abort(void);
int LED_IsTesting(void);

#endif /* LED_DRIVER_H */
