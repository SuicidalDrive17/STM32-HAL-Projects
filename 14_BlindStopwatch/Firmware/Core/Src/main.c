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
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
	STATE_IDLE,
	STATE_MODE_5S_CHOSEN,
	STATE_WAITING_TO_START_5S,
	STATE_RUNNING_5S,
	STATE_WAITING_TO_STOP_5S,
	STATE_EVALUATE_5S,
	STATE_MODE_10S_CHOSEN,
	STATE_WAITING_TO_START_10S,
	STATE_RUNNING_10S,
	STATE_WAITING_TO_STOP_10S,
	STATE_EVALUATE_10S,
	STATE_TIMEOUT
} GameState_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
GameState_t current_state = STATE_IDLE;
char tx_buffer[64];

volatile uint32_t final_time = 0;
volatile uint32_t last_button_press = 0;

int32_t current_score = 0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  snprintf(tx_buffer, sizeof(tx_buffer), "\r\n*** BLIND STOPWATCH ***\r\nSelect Mode: Press 5S or 10S Button to begin!\r\n");
  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  switch (current_state)
	  {
	  case STATE_IDLE:
	  case STATE_WAITING_TO_START_5S:
	  case STATE_WAITING_TO_STOP_5S:
	  case STATE_WAITING_TO_START_10S:
	  case STATE_WAITING_TO_STOP_10S:
		  // Do nothing. The CPU just spins here while the hardware waits for a button press
		  break;

	  case STATE_MODE_5S_CHOSEN:
		  // Turn OFF LEDs
		  HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(YELLOW_LED_GPIO_Port, YELLOW_LED_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);

		  // Text
		  snprintf(tx_buffer, sizeof(tx_buffer), "\r\n--- 5S MODE ---\r\nClick STOPWATCH button to START!\r\n");
		  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);

		  current_state = STATE_WAITING_TO_START_5S;
		  break;

	  case STATE_MODE_10S_CHOSEN:
		  // Turn OFF LEDs
		  HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(YELLOW_LED_GPIO_Port, YELLOW_LED_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);

		  // Text
		  snprintf(tx_buffer, sizeof(tx_buffer), "\r\n--- 10S MODE ---\r\nClick STOPWATCH button to START!\r\n");
		  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);

		  current_state = STATE_WAITING_TO_START_10S;
		  break;

	  case STATE_EVALUATE_5S:
	  {
  		  int32_t target_5S = 5000000;
  		  int32_t result_5S = abs(final_time - target_5S);

  		  // Extracting seconds and miliseconds for printing
  		  uint32_t sec = final_time / 1000000;
  		  uint32_t ms = (final_time % 1000000) / 1000;

  		  // 500000 = 500ms = 0.5s
  		  // 100000 = 1s
  		  if (result_5S < 500000)
  		  {
  			  // Green LED user gets 1000 points
  			  current_score += 1000;
  			  HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
  			  snprintf(tx_buffer, sizeof(tx_buffer), "Time: %lu.%03lus | GREEN (+1000 pts) | Score: %ld\r\n", sec, ms, current_score);
  		  }
  		  else if (result_5S >= 500000 && result_5S < 1000000)
  		  {
  			  // Yellow LED user gets 0 points
  			  HAL_GPIO_WritePin(YELLOW_LED_GPIO_Port, YELLOW_LED_Pin, GPIO_PIN_SET);
  			  snprintf(tx_buffer, sizeof(tx_buffer), "Time: %lu.%03lus | YELLOW (+0 pts)| Score: %ld\r\n", sec, ms, current_score);
  		  }
  		  else if (result_5S >= 1000000)
  		  {
  			  // Red LED user gets -500 points
  			  current_score -= 500;
  			  HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
  			  snprintf(tx_buffer, sizeof(tx_buffer), "Time: %lu.%03lus | RED (-500 pts) | Score: %ld\r\n", sec, ms, current_score);
  		  }
  		  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);

  		  // Win/loss reset condition
  		  if (current_score >= 5000)
  		  {
  			  snprintf(tx_buffer, sizeof(tx_buffer), "\r\n*** YOU WIN! 5000 POINTS REACHED! ***\r\n\r\n");
  			  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);
  			  current_score = 0;
  		  }
  		  else if (current_score <= -2000)
  		  {
  			  snprintf(tx_buffer, sizeof(tx_buffer), "\r\n*** GAME OVER. YOU DROPPED TO -2000 POINTS. ***\r\n\r\n");
  			  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);
  		  }

  		  // Prompt the user for the next round
		  snprintf(tx_buffer, sizeof(tx_buffer), "\r\nSelect Mode: Press 5S or 10S Button to begin!\r\n");
		  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);

  		  current_state = STATE_IDLE;	// Reset for the next round
  		  break;

	  }

	  	  case STATE_EVALUATE_10S:
	  	  {
	  		  int32_t target_10S = 10000000;
			  int32_t result_10S = abs(final_time - target_10S);

			  uint32_t sec = final_time / 1000000;
			  uint32_t ms = (final_time % 1000000) / 1000;

			  if (result_10S < 1000000)
			  {
				  // Green LED and +1000 pts
				  current_score += 1000;
				  HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
				  snprintf(tx_buffer, sizeof(tx_buffer), "Time: %lu.%03lus | GREEN (+1000 pts) | Score: %ld\r\n", sec, ms, current_score);
			  }
			  else if (result_10S <= 1000000 && result_10S < 2000000)
			  {
				  // Yellow LED and +0 pts
				  HAL_GPIO_WritePin(YELLOW_LED_GPIO_Port, YELLOW_LED_Pin, GPIO_PIN_SET);
				  snprintf(tx_buffer, sizeof(tx_buffer), "Time: %lu.%03lus | YELLOW (+0 pts) | Score: %ld\r\n", sec, ms, current_score);
			  }
			  else if (result_10S >= 2000000)
			  {
				  // Red LED and -500 pts
				  current_score -= 500;
				  HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
				  snprintf(tx_buffer, sizeof(tx_buffer), "Time: %lu.%03lus | RED (-500 pts) | Score: %ld\r\n", sec, ms, current_score);
			  }
			  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);

			  // Win/loss reset condition
			  if (current_score >= 5000)
			  {
				  snprintf(tx_buffer, sizeof(tx_buffer), "\r\n*** YOU WIN! 5000 POINTS REACHED! ***\r\n\r\n");
				  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);
				  current_score = 0;
			  }
			  else if (current_score <= -2000)
			  {
				  snprintf(tx_buffer, sizeof(tx_buffer), "\r\n*** GAME OVER. YOU DROPPED TO -2000 POINTS. ***\r\n\r\n");
				  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);
				  current_score = 0;
			  }

			  // Prompt the user for the next round
			  snprintf(tx_buffer, sizeof(tx_buffer), "\r\nSelect Mode: Press 5S or 10S Button to begin!\r\n");
			  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);

			  current_state = STATE_IDLE;
			  break;
	  	  }

	  	  case STATE_RUNNING_5S:
	  		  snprintf(tx_buffer, sizeof(tx_buffer), "Timer is running... Count 5s!\r\n");
	  		  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);
	  		  current_state = STATE_WAITING_TO_STOP_5S;
	  		  break;

	  	  case STATE_RUNNING_10S:
	  		  snprintf(tx_buffer, sizeof(tx_buffer), "Timer is running... Count 10s!\r\n");
	  		  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);
	  		  current_state = STATE_WAITING_TO_STOP_10S;
	  		  break;

	  	  case STATE_TIMEOUT:
	  		  current_score -= 500;
	  		  HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
	  		  snprintf(tx_buffer, sizeof(tx_buffer), "TIMEOUT!\r\n");
	  		  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);

	  		  if (current_score <= -2000)
	  		  {
	  			  snprintf(tx_buffer, sizeof(tx_buffer), "GAME OVER. YOU DROPPED TO -2000 POINTS\r\n\r\n");
	  			  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);
	  			  current_score = 0;
	  		  }

	  		  // Prompt the user for the next round
			  snprintf(tx_buffer, sizeof(tx_buffer), "\r\nSelect Mode: Press 5S or 10S Button to begin!\r\n");
			  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);

	  		  current_state = STATE_IDLE;
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
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 79;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 14999999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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

  /*Configure GPIO pins : BTN_5S_Pin BTN_10S_Pin STOPWATCH_BTN_Pin */
  GPIO_InitStruct.Pin = BTN_5S_Pin|BTN_10S_Pin|STOPWATCH_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

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

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	// Debounce filter
	uint32_t current_time = HAL_GetTick();
	if (current_time - last_button_press < 250)	// Ignore inputs faster than 250ms
	{
		return;
	}
	last_button_press = current_time;

	// 5-Second mode
	if (GPIO_Pin == BTN_5S_Pin && current_state == STATE_IDLE)
	{
		TIM2->CNT = 0;
		current_state = STATE_MODE_5S_CHOSEN;
	}
	// 10-Second mode
	else if (GPIO_Pin == BTN_10S_Pin && current_state == STATE_IDLE)
	{
		TIM2->CNT = 0;
		current_state = STATE_MODE_10S_CHOSEN;
	}

	// Stopwatch button
	else if (GPIO_Pin == STOPWATCH_BTN_Pin)
	{
		// Starting the timer
		if (current_state == STATE_WAITING_TO_START_5S)
		{
			TIM2->CNT = 0;
			HAL_TIM_Base_Start_IT(&htim2);
			current_state = STATE_RUNNING_5S;	// Signals main loop to print "Timer running"
		}
		else if (current_state == STATE_WAITING_TO_START_10S)
		{
			TIM2->CNT = 0;
			HAL_TIM_Base_Start_IT(&htim2);
			current_state = STATE_RUNNING_10S;
		}

		// Stopping the timer
		else if (current_state == STATE_WAITING_TO_STOP_5S)
		{
			final_time = TIM2->CNT;
			HAL_TIM_Base_Stop_IT(&htim2);
			current_state = STATE_EVALUATE_5S;	// Signals main loop to calculate score
		}
		else if (current_state == STATE_WAITING_TO_STOP_10S)
		{
			final_time = TIM2->CNT;
			HAL_TIM_Base_Stop_IT(&htim2);
			current_state = STATE_EVALUATE_10S;
		}
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM2)
	{
		// 15 sec ARR limit was hit
		HAL_TIM_Base_Stop_IT(&htim2);
		current_state = STATE_TIMEOUT;
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
