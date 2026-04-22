// Copyright 2026 Gruby
#ifndef SLAVE_FW_SIL3_1_CORE_INC_IO_MANAGER_H_
#define INC_IO_MANAGER_H_

#include "main.h"

void IO_Init(void);
void IO_SetLeds(uint8_t count);
void IO_AllOff(void);
void IO_RunSelfTest(void);

#endif  // SLAVE_FW_SIL3_1_CORE_INC_IO_MANAGER_H_
