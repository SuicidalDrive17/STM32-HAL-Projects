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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
	MODE_SENDING,		// Pot controls LEDs
	MODE_RECEIVING		// UART commands control LEDs
} GameMode_t;

typedef enum {
STATE_SAFE,		// Green led
STATE_MID,		// Yellow led
STATE_WARNING,	// Red led
STATE_ALARM		// Blinking every 250ms red led
} SystemState_t;

SystemState_t currentState = STATE_SAFE;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */
volatile GameMode_t current_mode = MODE_SENDING;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile uint16_t data_buffer[1];
char tx_buffer[64];

char rx_buffer[7];
volatile uint8_t command_ready = 0;
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  // 1. Sanity check boot message
  char boot_msg[] = "SYSTEM BOOTING...\r\n";
  HAL_UART_Transmit(&huart2, (uint8_t*)boot_msg, strlen(boot_msg), 1000);

  // 2. Calibrate the L4 ADC before starting
  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);

  // 3. Start the DMA stream
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)data_buffer, 1);

  // 4. Surgically kill the DMA interrupts to prevent the CPU crash
  __HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_TC | DMA_IT_HT | DMA_IT_TE);

  HAL_UART_Receive_DMA(&huart2, (uint8_t*)rx_buffer, sizeof(rx_buffer));
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
while (1)
{
	// The mode announcer
	static GameMode_t last_known_mode = MODE_SENDING;

	if (current_mode != last_known_mode)
	{
		if (current_mode == MODE_RECEIVING)
		{
			static char msg[] = "\r\n--- ENTERED RECEIVING MODE ---\r\n";
			HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
		}
		else if (current_mode == MODE_SENDING)
		{
			static char msg[] = "\r\n--- ENTERED SENDING MODE ---\r\n";
			HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
		}
	}

	last_known_mode = current_mode;

	switch (current_mode)
	{
		case MODE_SENDING:
		{
			// 1. State Machine with Hysteresis (Deadband filter)
			if (currentState == STATE_SAFE)
			{
				if (data_buffer[0] > 1250) currentState = STATE_MID;
			}
			else if (currentState == STATE_MID)
			{
				if (data_buffer[0] < 1150) currentState = STATE_SAFE;
				else if (data_buffer[0] > 2450) currentState = STATE_WARNING;
			}
			else if (currentState == STATE_WARNING)
			{
				if (data_buffer[0] < 2350) currentState = STATE_MID;
				else if (data_buffer[0] > 3650) currentState = STATE_ALARM;
			}
			else if (currentState == STATE_ALARM)
			{
				if (data_buffer[0] < 3550) currentState = STATE_WARNING;
			}

			// 2. Smart Value & State Tracking
			static uint16_t last_pot_value = 0xFFFF;
			static SystemState_t last_sent_state = (SystemState_t)99;

			// FIX: Freeze the volatile memory into a snapshot for this loop iteration
			uint16_t current_pot = data_buffer[0];

			// Calculate absolute difference using the frozen snapshot
			uint16_t value_diff = (current_pot > last_pot_value) ? (current_pot - last_pot_value) : (last_pot_value - current_pot);

			// Transmit ONLY if you turned the dial more than 100 units, OR if the LED state changed
			if (value_diff > 50 || currentState != last_sent_state)
			{
				if (currentState == STATE_SAFE)
				{
					snprintf(tx_buffer, sizeof(tx_buffer), "STATE SAFE | Pot:%u\r\n", current_pot);
				}
				else if (currentState == STATE_MID)
				{
					snprintf(tx_buffer, sizeof(tx_buffer), "STATE MID | Pot:%u\r\n", current_pot);
				}
				else if (currentState == STATE_WARNING)
				{
					snprintf(tx_buffer, sizeof(tx_buffer), "STATE WARNING | Pot:%u\r\n", current_pot);
				}
				else if (currentState == STATE_ALARM)
				{
					snprintf(tx_buffer, sizeof(tx_buffer), "STATE ALARM | Pot:%u\r\n", current_pot);
				}

				// Fire the DMA transmission
				HAL_UART_Transmit_DMA(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer));

				// Lock the gates using the snapshot
				last_pot_value = current_pot;
				last_sent_state = currentState;
			}

			switch (currentState)
			{
				case STATE_SAFE:
					// Green led
					HAL_GPIO_WritePin(GPIOC, GREEN_LED_Pin, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOC, YELLOW_LED_Pin, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, RED_LED_Pin, GPIO_PIN_RESET);
					break;

				case STATE_MID:
					// Yellow led
					HAL_GPIO_WritePin(GPIOC, GREEN_LED_Pin, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, YELLOW_LED_Pin, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOC, RED_LED_Pin, GPIO_PIN_RESET);
					break;

				case STATE_WARNING:
					// Red led
					HAL_GPIO_WritePin(GPIOC, GREEN_LED_Pin, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, YELLOW_LED_Pin, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOC, RED_LED_Pin, GPIO_PIN_SET);
					break;

				case STATE_ALARM:
					// Blinking red led every 250 ms
					// static keyword means it won't get erased when the loop restarts
					static uint32_t last_toggle_time = 0;

					if (HAL_GetTick() - last_toggle_time >= 250)
					{
						HAL_GPIO_WritePin(GPIOC, GREEN_LED_Pin, GPIO_PIN_RESET);
						HAL_GPIO_WritePin(GPIOC, YELLOW_LED_Pin, GPIO_PIN_RESET);
						HAL_GPIO_TogglePin(GPIOC, RED_LED_Pin);

						last_toggle_time = HAL_GetTick();
					}
				break;
			}
		}
			break;

			case MODE_RECEIVING:
				if (command_ready == 1)
				{
					// Instantly lower the flag
					command_ready = 0;

					// Parse the prefix
					if (strncmp((char*)rx_buffer, "LCD:", 4) == 0)
					{
						// 3. Read specific array indices.
						// rx_buffer[0] = 'L', [1] = 'C', [2] = 'D', [3] = ':'
						// rx_buffer[4] = Green, [5] = Yellow, [6] = Red

						HAL_GPIO_WritePin(GPIOC, GREEN_LED_Pin, (rx_buffer[4] == '1') ? GPIO_PIN_SET : GPIO_PIN_RESET);
						HAL_GPIO_WritePin(GPIOC, YELLOW_LED_Pin, (rx_buffer[5] == '1') ? GPIO_PIN_SET : GPIO_PIN_RESET);
						HAL_GPIO_WritePin(GPIOC, RED_LED_Pin, (rx_buffer[6] == '1') ? GPIO_PIN_SET : GPIO_PIN_RESET);
					}
				}
			break;
		}

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GREEN_LED_Pin|YELLOW_LED_Pin|RED_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : GREEN_LED_Pin YELLOW_LED_Pin RED_LED_Pin */
  GPIO_InitStruct.Pin = GREEN_LED_Pin|YELLOW_LED_Pin|RED_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN_Pin */
  GPIO_InitStruct.Pin = BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2)
	{
		// Raise the flag so the while loop knows new data arrived
		command_ready = 1;

		// Re-arm the DMA to listen for the next 7 chars
		HAL_UART_Receive_DMA(huart, (uint8_t*)rx_buffer, 7);
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	// Check if the interrupt came from PC9 (BTN_Pin)
	if (GPIO_Pin == BTN_Pin)
	{
		static uint32_t last_button_press = 0;
		uint32_t current_time = HAL_GetTick();

		// Debounce 250ms must pass
		if (current_time - last_button_press > 250)
		{
			if (current_mode == MODE_SENDING)
			{
				current_mode = MODE_RECEIVING;
			}
			else if (current_mode == MODE_RECEIVING)
			{
				current_mode = MODE_SENDING;
			}

			last_button_press = current_time;
		}
	}
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
