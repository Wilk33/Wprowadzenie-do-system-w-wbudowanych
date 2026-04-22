// Copyright 2026 Gruby
include "uart_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- Inicjalizacja --- */
void UARTM_Init(UART_Manager_t *u, UART_HandleTypeDef *huart,
                UART_Backend_t backend, UART_Mode_t mode, bool echo) {
  u->huart = huart;
  u->backend = backend;
  u->mode = mode;
  u->echo = echo;
  u->idx = 0;
  u->ready = false;
  u->value1 = 0;
  u->value2 = 0;
  memset(u->rx_buf, 0, sizeof(u->rx_buf));
  memset(u->value, 0, sizeof(u->value));
  if (backend == UART_BACKEND_USART && huart != NULL)
    HAL_UART_Receive_IT(huart, (uint8_t *)&u->rx_buf[u->idx], 1);
}

/* --- Wysyłanie pojedynczego znaku --- */
void UARTM_TransmitChar(UART_Manager_t *u, char c) {
  HAL_UART_Transmit(u->huart, (uint8_t *)&c, 1, HAL_MAX_DELAY);
}

/* --- Wysyłanie stringa --- */
void UARTM_TransmitString(UART_Manager_t *u, const char *s) {
  HAL_UART_Transmit(u->huart, (uint8_t *)s, strlen(s), HAL_MAX_DELAY);
}

/* --- Czy coś odebrano --- */
bool UARTM_Available(UART_Manager_t *u) { return u->ready; }

/* --- Pobierz ostatni znak --- */
int UARTM_GetChar(UART_Manager_t *u) {
  u->ready = false;
  return u->value1;
}

/* --- Pobierz kod ASCII ostatniego znaku --- */
int UARTM_GetAscii(UART_Manager_t *u) {
  u->ready = false;
  return (int)u->rx_buf[0];
}

/* --- Pobierz pełny string (gdy tryb LINE) --- */
double UARTM_GetString(UART_Manager_t *u) {
  u->ready = false;
  double base = (double)u->value1;
  double frac = 0.0;
  int v = u->value2;
  int rev = 0;
  int div = 1;

  // Odwróć kolejność cyfr w value2 (np. 12 → 21)
  while (v > 0) {
    rev = rev * 10 + (v % 10);
    v /= 10;
  }

  // Teraz budujemy ułamek we właściwej kolejności
  while (rev > 0) {
    div *= 10;
    frac += (rev % 10) / (double)div;
    rev /= 10;
  }

  return base + frac;
}

char *UARTM_GetToString(UART_Manager_t *u) {
  u->ready = false;
  return u->value;
}

/* --- Obsługa odbioru z przerwania UART --- */
void UARTM_IRQHandler(UART_Manager_t *u) {
  char c = u->rx_buf[u->idx];

  /* --- TRYB CHAR --- */
  if (u->mode == UART_MODE_CHAR) {
    u->ready = true;

    // zapis do value1 jako liczba (nie kod ASCII)
    u->value1 = (int)(c - '0');
    if (u->value1 < 0 || u->value1 > 9)
      u->value1 = 9999;

    if (u->echo) {
      char msg[32];
      sprintf(msg, "Char: '%c' ASCII: %d\r\n", c, (int)c);
      HAL_UART_Transmit(u->huart, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
    }
  } else if (u->mode == UART_MODE_LINE) {
    if (c == '\r' || c == '\n') {
      u->rx_buf[u->idx] = '\0';
      strcpy(u->value, u->rx_buf); // zachowaj cały ciąg znaków
      u->idx = 0;
      u->ready = true;

      u->value1 = 0;
      u->value2 = 0;
      bool valid = true;
      bool after_decimal = false;

      for (int i = 0; u->value[i] != '\0'; i++) {
        char ch = u->value[i];

        if (ch == '.' || ch == ',') {
          if (after_decimal) {
            valid = false;
            break;
          }
          after_decimal = true;
          continue;
        }

        if (ch >= '0' && ch <= '9') {
          int digit = ch - '0';
          if (!after_decimal)
            u->value1 = u->value1 * 10 + digit;
          else
            u->value2 = u->value2 * 10 + digit;
        } else {
          valid = false;
          break;
        }
      }

      if (!valid) {
        u->value1 = 0;
        u->value2 = 0;
      }

      if (u->echo) {
        char msg[128];
        if (valid)
          snprintf(msg, sizeof(msg),
                   "\r\nString: %s  -> Char: %d  -> Double: %d,%d\r\n",
                   u->value, u->value1, u->value1, u->value2);
        else
          snprintf(msg, sizeof(msg), "\r\nString: %s\r\n", u->value);

        HAL_UART_Transmit(u->huart, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
      }
    } else if (u->idx < sizeof(u->rx_buf) - 1) {
      if (u->echo) {
        // wyślij echo przez UART
        HAL_UART_Transmit(u->huart, (uint8_t *)&c, 1, HAL_MAX_DELAY);
      }
      u->idx++;
    }
  }

  // ponownie włącz odbiór tylko dla UART
  if (u->backend == UART_BACKEND_USART && u->huart != NULL) {
    HAL_UART_Receive_IT(u->huart, (uint8_t *)&u->rx_buf[u->idx], 1);
  }
}

void UARTM_SetEcho(UART_Manager_t *u, bool echo) { u->echo = echo; }

void UARTM_SetMode(UART_Manager_t *u, UART_Mode_t mode) {
  u->mode = mode;
  u->idx = 0;
  u->ready = false;
  u->value1 = 0;
  u->value2 = 0;
  memset(u->rx_buf, 0, sizeof(u->rx_buf));
}

void UARTM_SetBackend(UART_Manager_t *u, UART_Backend_t backend) {
  u->backend = backend;
  u->idx = 0;
  u->ready = false;
  u->value1 = 0;
  u->value2 = 0;
  memset(u->rx_buf, 0, sizeof(u->rx_buf));
}

/* --- Wysyłanie pojedynczego znaku lub liczby ASCII --- */
void UARTM_SendChar(UART_Manager_t *u, char c) {
  if (u == NULL)
    return;

  char msg[64];

  if (u->echo) {
    sprintf(msg, "ASCII: %c -> int: %d\r\n", c, (int)c);
  } else {
    sprintf(msg, "%c\r\n", c);
  }

  HAL_UART_Transmit(u->huart, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

/* --- Wysyłanie ciągłego tekstu (pełnego zdania) --- */
void UARTM_SendContinuous(UART_Manager_t *u, const char *text) {
  if (u == NULL || text == NULL)
    return;

  char msg[256];
  snprintf(msg, sizeof(msg), "%s\r\n", text);

  HAL_UART_Transmit(u->huart, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}
