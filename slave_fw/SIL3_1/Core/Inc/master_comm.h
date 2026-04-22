#ifndef INC_MASTER_COMM_H_
#define INC_MASTER_COMM_H_

#include "main.h"
#include <stdbool.h> // <-- DODAJ TĘ LINIĘ

#define MASTER_GRANT 0xA5
#define MASTER_DENY 0x5A

void MC_SendAuthRequest(uint8_t ledCount);
bool MC_CheckResponse(
    uint8_t *response); // Teraz kompilator będzie wiedział co to bool
void MC_IRQHandler(
    UART_HandleTypeDef *huart); // Upewnij się, że ten prototyp też tu jest

#endif
