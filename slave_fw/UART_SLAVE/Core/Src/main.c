/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_IDLE,
    STATE_WAITING_MASTER_AUTH,
    STATE_COMMAND_AUTHORIZED,
    STATE_ERROR,
    STATE_SAFETY,
    STATE_SAFETY_PERMANENT
} SlaveState_t;

typedef struct {
    uint8_t command;
    uint16_t crc;
} CommandFrame_t;

typedef struct {
    uint8_t authorized;
    uint16_t crc;
} AuthResponse_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CRC_POLY           0x1021
#define RX_BUFFER_SIZE     10
#define CMD_MIN_VALUE      '1'
#define CMD_MAX_VALUE      '8'
#define AUTH_TIMEOUT_MS    5000
/* FIX: Safety State timeout — non-blocking, measured in ms */
#define SAFETY_TIMEOUT_MS  3000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/*
 * FIX #1: Separate receive buffers for each UART.
 * Previously both used the same `rx_data` byte — a race condition
 * where UART1 interrupt could overwrite UART2 data before it was processed.
 */
uint8_t rx_data_uart1;
uint8_t rx_data_uart2;

char msg_start[]         = "STM32 Slave! Wyslij '1-8'...\r\n";
char msg_ok[]            = "Operacja wykonana!\r\n";
char msg_error_syntax[]  = "BLAD: Niepoprawna skladnia!\r\n";
char msg_error_auth[]    = "BLAD: Brak autoryzacji!\r\n";
char msg_auth_timeout[]  = "BLAD: Timeout autoryzacji!\r\n";

uint8_t leds = 0;

/*
 * FIX #2: volatile — slave_state is written from ISR context (RxCpltCallback)
 * and read in the main loop. Must be volatile to prevent compiler optimisation
 * from caching a stale value in a register.
 */
volatile SlaveState_t slave_state = STATE_IDLE;

uint32_t auth_timeout    = 0;
uint32_t safety_entry_time = 0;   /* FIX: timestamp for non-blocking Safety State */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void Update(uint8_t leds);
uint16_t CalculateCRC(uint8_t *data, uint8_t length);
void SendAuthorizationRequest(uint8_t command);
void HandleMasterResponse(uint8_t response);
void ValidateAndExecuteCommand(uint8_t cmd);
void PerformSelfTestStart(void);
void PerformSelfTestUpdate(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint16_t CalculateCRC(uint8_t *data, uint8_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t)(data[i] << 8);
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ CRC_POLY;
            else
                crc = crc << 1;
            crc &= 0xFFFF;
        }
    }
    return crc;
}

void SendAuthorizationRequest(uint8_t command)
{
    CommandFrame_t frame;
    frame.command = command;
    frame.crc = CalculateCRC(&frame.command, 1);

    char buffer[24];
    uint8_t len = (uint8_t)sprintf(buffer, "Aut_req:%d,%04X\r\n", command, frame.crc);
    HAL_UART_Transmit(&huart1, (uint8_t *)buffer, len, 100);

    slave_state  = STATE_WAITING_MASTER_AUTH;
    auth_timeout = HAL_GetTick();
}

/* ── Self-test ─────────────────────────────────────────────────────────────── */

typedef struct {
    int      current_led;
    uint32_t last_time;
    int      is_testing;
} SelfTest_State;

static SelfTest_State selftest_state = {0, 0, 0};

void PerformSelfTestStart(void)
{
    selftest_state.current_led = 0;
    selftest_state.is_testing  = 1;
    selftest_state.last_time   = HAL_GetTick();

    const char msg[] = "SELFTEST: Start\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, sizeof(msg) - 1, 100);
}

void PerformSelfTestUpdate(void)
{
    /*
     * FIX #3a: Self-test must not run while in any Safety State.
     * Previously it ran unconditionally at the top of while(1),
     * so LEDs still updated during STATE_SAFETY.
     */
    if (!selftest_state.is_testing)
        return;
    if (slave_state == STATE_SAFETY || slave_state == STATE_SAFETY_PERMANENT)
    {
        selftest_state.is_testing = 0;   /* abort test */
        Update(0);
        return;
    }

    /* FIX: use a single port/pin table declared once */
    static const GPIO_TypeDef *ports[] = {
        Led_1_GPIO_Port, Led_2_GPIO_Port, Led_3_GPIO_Port, Led_4_GPIO_Port,
        Led_5_GPIO_Port, Led_6_GPIO_Port, Led_7_GPIO_Port, Led_8_GPIO_Port
    };
    static const uint16_t pins[] = {
        Led_1_Pin, Led_2_Pin, Led_3_Pin, Led_4_Pin,
        Led_5_Pin, Led_6_Pin, Led_7_Pin, Led_8_Pin
    };

    uint32_t now = HAL_GetTick();
    if ((now - selftest_state.last_time) < 500)
        return;

    if (selftest_state.current_led < 8)
    {
        if (selftest_state.current_led > 0)
            HAL_GPIO_WritePin((GPIO_TypeDef *)ports[selftest_state.current_led - 1],
                              pins[selftest_state.current_led - 1], GPIO_PIN_RESET);

        HAL_GPIO_WritePin((GPIO_TypeDef *)ports[selftest_state.current_led],
                          pins[selftest_state.current_led], GPIO_PIN_SET);

        char led_msg[24];
        int len = sprintf(led_msg, "SELFTEST: LED %d ON\r\n", selftest_state.current_led + 1);
        HAL_UART_Transmit(&huart2, (uint8_t *)led_msg, (uint16_t)len, 100);

        selftest_state.current_led++;
        selftest_state.last_time = now;
    }
    else
    {
        HAL_GPIO_WritePin((GPIO_TypeDef *)ports[7], pins[7], GPIO_PIN_RESET);

        const char done[] = "SELFTEST: Done\r\n";
        HAL_UART_Transmit(&huart2, (uint8_t *)done, sizeof(done) - 1, 100);

        selftest_state.is_testing = 0;
        slave_state = STATE_IDLE;
    }
}

/* ── Command dispatcher ─────────────────────────────────────────────────────── */

void ValidateAndExecuteCommand(uint8_t rx_byte)
{

	if (slave_state == STATE_SAFETY)
	{
	    const char *m = "BLAD: Trwa tymczasowy SAFETY STATE - komenda zignorowana!\r\n";
	    HAL_UART_Transmit(&huart2, (uint8_t *)m, strlen(m), 100);
	    return;
	}

    if (slave_state == STATE_SAFETY_PERMANENT)
    {
        if (rx_byte == 'p' || rx_byte == 'P')
        {
            slave_state = STATE_IDLE;
            const char *m = "SAFETY PERMANENT -> IDLE\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t *)m, strlen(m), 100);
        }
        else
        {
            const char *m = "BLAD: SAFETY PERMANENT aktywny!\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t *)m, strlen(m), 100);
        }
        return;   /* all other commands blocked */
    }

    /* Self-test */
    if (rx_byte == 't' || rx_byte == 'T')
    {
        PerformSelfTestStart();
        return;
    }

    /* Safety State (temporary, auto-recovers after SAFETY_TIMEOUT_MS) */
    if (rx_byte == 's' || rx_byte == 'S')
    {
        slave_state       = STATE_SAFETY;
        safety_entry_time = HAL_GetTick();   /* FIX: record entry time */
        Update(0);
        const char *m = "SAFETY STATE\r\n";
        HAL_UART_Transmit(&huart2, (uint8_t *)m, strlen(m), 100);
        return;
    }

    /* Safety State Permanent */
    if (rx_byte == 'p' || rx_byte == 'P')
    {
        slave_state = STATE_SAFETY_PERMANENT;
        Update(0);
        const char *m = "SAFETY STATE PERMANENT\r\n";
        HAL_UART_Transmit(&huart2, (uint8_t *)m, strlen(m), 100);
        return;
    }

    /*
     * FIX #5: Software reset — flush UART before resetting.
     * HAL_Delay() after the message is replaced by a proper UART wait:
     * HAL_UART_Transmit is synchronous (blocking until TX complete) at 100ms
     * timeout, which is enough for a short message at 115200 baud.
     * NVIC_SystemReset() then runs cleanly.
     */
    if (rx_byte == 'r' || rx_byte == 'R')
    {
        const char *m = "SOFTWARE RESET\r\n";
        HAL_UART_Transmit(&huart2, (uint8_t *)m, strlen(m), 200);
        /* Wait for the UART TX shift register to empty */
        while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == RESET) {}
        NVIC_SystemReset();
        return;  /* never reached */
    }

    /* Normal LED commands 1-8 */
    if (rx_byte >= '1' && rx_byte <= '8')
    {
        uint8_t command = rx_byte - '0';
        leds = command;
        SendAuthorizationRequest(command);

        char cmd_msg[32];
        int len = sprintf(cmd_msg, "Komenda %d - czekam...\r\n", command);
        HAL_UART_Transmit(&huart2, (uint8_t *)cmd_msg, (uint16_t)len, 100);
        return;
    }

    /* Unknown byte */
    HAL_UART_Transmit(&huart2, (uint8_t *)msg_error_syntax, strlen(msg_error_syntax), 100);
}

void HandleMasterResponse(uint8_t response)
{
    if (slave_state != STATE_WAITING_MASTER_AUTH)
        return;

    /*
     * FIX #6: Pick one protocol — ASCII '1'/'0' from master.
     * The previous code checked both '1' (ASCII 49) and 1 (integer 1)
     * with ||, which is ambiguous. Standardised to ASCII here.
     * Change to `response == 1` / `response == 0` if master sends raw bytes.
     */
    if (response == '1')
    {
        Update(leds);
        HAL_UART_Transmit(&huart2, (uint8_t *)msg_ok, strlen(msg_ok), 100);
        slave_state = STATE_COMMAND_AUTHORIZED;
    }
    else if (response == '0')
    {
        HAL_UART_Transmit(&huart2, (uint8_t *)msg_error_auth, strlen(msg_error_auth), 100);
        slave_state = STATE_ERROR;
    }
    else
    {
        slave_state = STATE_IDLE;
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
    HAL_UART_Transmit(&huart2, (uint8_t *)msg_start, strlen(msg_start), 100);

    /* FIX: each UART gets its own receive buffer */
    HAL_UART_Receive_IT(&huart1, &rx_data_uart1, 1);
    HAL_UART_Receive_IT(&huart2, &rx_data_uart2, 1);

    slave_state = STATE_IDLE;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
      {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
            if (slave_state == STATE_SAFETY)
            {
                if ((HAL_GetTick() - safety_entry_time) >= SAFETY_TIMEOUT_MS)
                {
                    slave_state = STATE_IDLE;
                    const char *m = "SAFETY STATE -> IDLE\r\n";
                    HAL_UART_Transmit(&huart2, (uint8_t *)m, strlen(m), 100);
                }
                continue; /* Pomija resztę pętli w trybie Safety */
            }

            /* Aktualizacja Self-Testu */
            PerformSelfTestUpdate();

            /* Timeout autoryzacji */
            if (slave_state == STATE_WAITING_MASTER_AUTH)
            {
                if ((HAL_GetTick() - auth_timeout) > AUTH_TIMEOUT_MS)
                {
                    HAL_UART_Transmit(&huart2, (uint8_t *)msg_auth_timeout,
                                      strlen(msg_auth_timeout), 100);
                    slave_state = STATE_IDLE;
                }
            }
      }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_10;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, Led_5_Pin|Led_3_Pin|Led_4_Pin|Led_6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Led_1_Pin|Led_7_Pin|Led_2_Pin|Led_8_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : Led_5_Pin Led_3_Pin Led_4_Pin Led_6_Pin */
  GPIO_InitStruct.Pin = Led_5_Pin|Led_3_Pin|Led_4_Pin|Led_6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : Led_1_Pin Led_7_Pin Led_2_Pin Led_8_Pin */
  GPIO_InitStruct.Pin = Led_1_Pin|Led_7_Pin|Led_2_Pin|Led_8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* ── UART callbacks ────────────────────────────────────────────────────────── */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        ValidateAndExecuteCommand(rx_data_uart2);
        HAL_UART_Receive_IT(&huart2, &rx_data_uart2, 1);
    }
    else if (huart->Instance == USART1)
    {
        char debug_msg[24];
        int len = sprintf(debug_msg, "UART1_RX:%d\r\n", rx_data_uart1);
        HAL_UART_Transmit(&huart2, (uint8_t *)debug_msg, (uint16_t)len, 100);

        HandleMasterResponse(rx_data_uart1);
        HAL_UART_Receive_IT(&huart1, &rx_data_uart1, 1);
    }
}

/* ── LED output ────────────────────────────────────────────────────────────── */
void Update(uint8_t status)
{
    HAL_GPIO_WritePin(Led_1_GPIO_Port, Led_1_Pin, (status == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_2_GPIO_Port, Led_2_Pin, (status == 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_3_GPIO_Port, Led_3_Pin, (status == 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_4_GPIO_Port, Led_4_Pin, (status == 4) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_5_GPIO_Port, Led_5_Pin, (status == 5) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_6_GPIO_Port, Led_6_Pin, (status == 6) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_7_GPIO_Port, Led_7_Pin, (status == 7) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Led_8_GPIO_Port, Led_8_Pin, (status == 8) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
