// Copyright 2026 Tobiasz_Kandziora
// Ten moduł odpowiada za odliczanie czasu Watchdoga i weryfikację sum CRC (Heartbeat).
#ifndef SLAVE_FW_UART_SLAVE_CORE_INC_SIL_WATCHDOG_H_
#define SLAVE_FW_UART_SLAVE_CORE_INC_SIL_WATCHDOG_H_

#include <stdint.h>

#define PING_TIMEOUT_MS 1000

void SIL_Watchdog_Init(void);
void SIL_Watchdog_Feed(void);
int SIL_Watchdog_IsExpired(void);

uint16_t SIL_CalculateCRC(uint8_t *data, uint8_t length);
int SIL_ProcessPingFrame(uint8_t *buffer, uint8_t *last_seq,
                         uint8_t *first_ping);

#endif  // SLAVE_FW_UART_SLAVE_CORE_INC_SIL_WATCHDOG_H_
