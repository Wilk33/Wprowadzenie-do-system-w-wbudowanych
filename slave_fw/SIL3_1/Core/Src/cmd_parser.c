// Copyright 2026 Gruby
#include "cmd_parser.h"
#include <string.h>

CP_Result_t CP_Parse(const char *rawStr) {
  CP_Result_t result = {CMD_INVALID, 0};

  if (rawStr == NULL || rawStr[0] == '\0')
    return result;

  if (strcmp(rawStr, "START") == 0) {
    result.type = CMD_START;
  } else if (strcmp(rawStr, "STOP") == 0) {
    result.type = CMD_STOP;
  } else if (rawStr[0] == 'L' && rawStr[1] >= '1' && rawStr[1] <= '8' &&
             rawStr[2] == '\0') {
    result.type = CMD_LED_SET;
    result.ledCount = rawStr[1] - '0';
  }
  return result;
}