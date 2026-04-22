// Copyright 2026 Gruby
#pragma once
#include "main.h"
#include <stdbool.h>

/* Konfiguracja bufora odbioru */
#define UART_RX_BUF_SIZE 64

typedef enum {
  UART_MODE_CHAR, // każdy znak osobno
  UART_MODE_LINE  // pełna linia po Enterze
} UART_Mode_t;

typedef enum { UART_BACKEND_USART, UART_BACKEND_USB_CDC } UART_Backend_t;

typedef struct {
  UART_HandleTypeDef *huart;
  UART_Backend_t backend;
  char rx_buf[UART_RX_BUF_SIZE];
  uint8_t idx;
  UART_Mode_t mode;
  bool echo;  // czy wypisywać to, co wpisujesz w terminalu
  bool ready; // flaga: czy gotowy pełny string
  int value1;
  int value2;
  char value[UART_RX_BUF_SIZE];
} UART_Manager_t;

/* --- Funkcje interfejsu --- */
void UARTM_Init(UART_Manager_t *u, UART_HandleTypeDef *huart,
                UART_Backend_t backend, UART_Mode_t mode, bool echo);
void UARTM_TransmitChar(UART_Manager_t *u, char c);
void UARTM_TransmitString(UART_Manager_t *u, const char *s);
bool UARTM_Available(UART_Manager_t *u); // czy coś odebrano
int UARTM_GetChar(UART_Manager_t *u);
int UARTM_GetAscii(UART_Manager_t *u);
double UARTM_GetString(UART_Manager_t *u);
char *UARTM_GetToString(UART_Manager_t *u);
void UARTM_IRQHandler(
    UART_Manager_t *u); // wywoływana w HAL_UART_RxCpltCallback
void UARTM_SetEcho(UART_Manager_t *u, bool echo);
void UARTM_SetMode(UART_Manager_t *u, UART_Mode_t mode);
void UARTM_SetBackend(UART_Manager_t *u, UART_Backend_t backend);
void UARTM_SendChar(UART_Manager_t *u, char c);
void UARTM_SendContinuous(UART_Manager_t *u, const char *text);
