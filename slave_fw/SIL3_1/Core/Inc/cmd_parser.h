#ifndef INC_CMD_PARSER_H_
#define INC_CMD_PARSER_H_

#include <stdbool.h>
#include <stdint.h>


typedef enum { CMD_INVALID, CMD_START, CMD_STOP, CMD_LED_SET } CP_CommandType_t;

typedef struct {
  CP_CommandType_t type;
  uint8_t ledCount;
} CP_Result_t;

CP_Result_t CP_Parse(const char *rawStr);

#endif