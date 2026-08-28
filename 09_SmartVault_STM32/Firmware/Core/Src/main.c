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
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
	STATE_DISARMED,
	STATE_ARMED,
	STATE_ALARM,
	STATE_DISARMING
} SystemState_t;

SystemState_t currentState = STATE_DISARMED;

// Variables to hold the security PIN
char saved_pin[5] = "0000";		// The 4-digit PIN (plus null terminator)
char entered_pin[5] = "";		// What the user is currently typing
uint8_t pin_index = 0;			// Keeps track of how many digits were typed
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_rx;
DMA_HandleTypeDef hdma_i2c1_tx;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */
volatile char new_key = '\0';
uint8_t i2c_rx_buffer[6];	// Holds X, Y and Z accelerometer bytes
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
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
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  // Start the 5ms hardware timer to scan the keypad in the background
  HAL_TIM_Base_Start_IT(&htim2);

  // Wake up the MPU9250 (Write 0x00 to register 0x6B)
  // Note: AD0 is wired to 3.3V, so the address is 0x69 (shifted left by 1)
  uint8_t wake_data = 0x00;
  HAL_I2C_Mem_Write(&hi2c1, (0x69 << 1), 0x6B, 1, &wake_data, 1, 100);

  // Start the continuous DMA read of the 6 accelerometer registers (starting at 0x3B)
  // This will run silently in the background forever!
  HAL_I2C_Mem_Read_DMA(&hi2c1, (0x69 << 1), 0x3B, 1, i2c_rx_buffer, 6);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // 1. Check if the background interrupt handed us a new key
	  char current_key = '\0';
	  if (new_key != '\0')
	  {
		  current_key = new_key;
		  new_key = '\0';	// Clear the global flag so we don't read it twice
	  }

	  // 2. Security State Machine
	  switch (currentState)
	  {
	  	  case STATE_DISARMED:
	  		  // Buzzer OFF
	  		  HAL_GPIO_WritePin(GPIOC, BUZZER_Pin, GPIO_PIN_RESET);

	  		  if (current_key != '\0')
	  		  {
	  			  // print every key pressed
	  			  static char key_msg[30];
	  			  sprintf(key_msg, "[DISARMED] Key: %c\r\n", current_key);
	  			  HAL_UART_Transmit_DMA(&huart2, (uint8_t*)key_msg, strlen(key_msg));

	  			  if (current_key == '*')
	  			  {
	  				  // Star button clears everything so you can start over
	  				  pin_index = 0;
	  				  memset(entered_pin, 0, sizeof(entered_pin));
	  			  }
	  			  else if (current_key == '#')
	  			  {
	  				  if (pin_index == 4)
	  				  {
	  					  // Save the new PIN and ARM the system
	  					  strcpy(saved_pin, entered_pin);
	  					  pin_index = 0;
	  					  memset(entered_pin, 0, sizeof(entered_pin));

	  					  static char arm_msg[] = "\r\n>>> SYSTEM ARMED <<<\r\n";
	  					  HAL_UART_Transmit_DMA(&huart2, (uint8_t*)arm_msg, strlen(arm_msg));

	  					  // Force wake the IMU every single time we arm it!
                          // This guarantees it works even if it lost power and rebooted while disarmed.
	  					  uint8_t wake_data = 0x00;
	  					  HAL_I2C_Mem_Write(&hi2c1, (0x69 << 1), 0x6B, 1, &wake_data, 1, 100);

	  					  currentState = STATE_ARMED;
	  				  }
	  			  }
	  			  else
	  			  {
	  				  // It's a number so we add it to our array if we have space
	  				  if (pin_index < 4)
	  				  {
	  					  entered_pin[pin_index] = current_key;
	  					  pin_index++;
	  					  entered_pin[pin_index] = '\0';	// Keep the string null-terminated
	  				  }
	  			  }
	  		  }
	  		  break;

	  	  case STATE_ARMED:
	  	  {
	  		  // DMA trigger (runs 20 times per second)
	  		  static uint32_t last_imu_read = 0;
	  		  if (HAL_GetTick() - last_imu_read > 50)
	  		  {
	  			  // Only trigger DMA if the I2C peripheral is completely healthy
	  			  if (HAL_I2C_GetState(&hi2c1) == HAL_I2C_STATE_READY)
	  			  {
		  			  // If a wire jiggles and fails, it just tries again 50ms later
		  			  HAL_I2C_Mem_Read_DMA(&hi2c1, (0x69 << 1), 0x3B, 1, i2c_rx_buffer, 6);
		  			  last_imu_read = HAL_GetTick();
	  			  }
	  		  }

	  		  // 250ms UART print
	  		  static uint32_t last_print = 0;
	  		  if (HAL_GetTick() - last_print > 250)
	  		  {
	  			  // Re-calculate z_axis here ONLY for the printout
	  			  int16_t z_axis_print = (i2c_rx_buffer[4] << 8) | i2c_rx_buffer[5];
	  			  static char debug_msg[50];
	  			  sprintf(debug_msg, "Z-Axis: %d\r\n", z_axis_print);
	  			  HAL_UART_Transmit_DMA(&huart2, (uint8_t*)debug_msg, strlen(debug_msg));
	  			  last_print = HAL_GetTick();
	  		  }
	  		  break;
	  	  }

	  	  case STATE_ALARM:
	  		  // Sound the siren
	  		  HAL_GPIO_WritePin(GPIOC, BUZZER_Pin, GPIO_PIN_SET);

	  		  // Clear out any old keypad data so the user starts with a clean slate
	  		  pin_index = 0;
	  		  memset(entered_pin, 0, sizeof(entered_pin));

	  		  static char alarm_msg[] = "\r\n!!! MOVEMENT DETECTED - ALARM !!!\r\n";
	  		  HAL_UART_Transmit_DMA(&huart2, (uint8_t*)alarm_msg, strlen(alarm_msg));

	  		  currentState = STATE_DISARMING;
	  		  break;

	  	  case STATE_DISARMING:
	  		  // Buzzer is screaming. The user must type the exact PIN to shut it off
	  		  if (current_key != '\0')
	  		  {
	  			  static char panic_msg[30];
	  			  sprintf(panic_msg, "[DISARMING] Key: %c\r\n", current_key);

	  			  if (current_key == '*')
	  			  {
	  				  pin_index = 0;
	  				  memset(entered_pin, 0, sizeof(entered_pin));
	  			  }
	  			  else if (current_key == '#')
	  			  {
	  				  // They pressed enter. We use strcmp (String Compare) to check the password!
	  				  if (strcmp(entered_pin, saved_pin) == 0)
	  				  {
	  					  // Success
	  					  HAL_GPIO_WritePin(GPIOC, BUZZER_Pin, GPIO_PIN_RESET);

	  					  // Clear entry for the next time
	  					  pin_index = 0;
	  					  memset(entered_pin, 0, sizeof(entered_pin));

	  					  static char succ_msg[] = "\r\n>>> DISARMED SUCCESS <<<\r\n";
	  					  HAL_UART_Transmit_DMA(&huart2, (uint8_t*)succ_msg, strlen(succ_msg));
	  					  currentState = STATE_DISARMED;
	  				  }
	  				  else
	  				  {
	  					  // Wrong password
	  					  pin_index = 0;
	  					  memset(entered_pin, 0, sizeof(entered_pin));

	  					  static char err_msg[] = "\r\n>>> WRONG PIN <<<\r\n";
	  					  HAL_UART_Transmit_DMA(&huart2, (uint8_t*)err_msg, strlen(err_msg));
	  				  }
	  			  }
	  			  else
	  			  {
	  				  // Store the numbers as user type them
	  				  if (pin_index < 4)
	  				  {
	  					  entered_pin[pin_index] = current_key;
	  					  pin_index++;
	  					  entered_pin[pin_index] = '\0';
	  				  }
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10D19CE4;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  htim2.Init.Period = 4999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);
  /* DMA2_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel6_IRQn);
  /* DMA2_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel7_IRQn);

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
  HAL_GPIO_WritePin(GPIOC, COL_1_Pin|COL_2_Pin|COL_3_Pin|COL_4_Pin
                          |BUZZER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : COL_1_Pin COL_2_Pin COL_3_Pin COL_4_Pin
                           BUZZER_Pin */
  GPIO_InitStruct.Pin = COL_1_Pin|COL_2_Pin|COL_3_Pin|COL_4_Pin
                          |BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ROW_1_Pin ROW_2_Pin ROW_3_Pin ROW_4_Pin */
  GPIO_InitStruct.Pin = ROW_1_Pin|ROW_2_Pin|ROW_3_Pin|ROW_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // Ensure we are only running this for TIM2 (the 5ms timer)
    if (htim->Instance == TIM2)
    {
        // 1. The physical layout of your 4x4 keypad
        char keypad_map[4][4] = {
            {'1', '2', '3', 'A'},
            {'4', '5', '6', 'B'},
            {'7', '8', '9', 'C'},
            {'*', '0', '#', 'D'}
        };

        // 2. Static variable remembers the column across interrupts
        static uint8_t current_col = 0;

        // 3. Turn OFF all columns first to prevent crossed signals
        HAL_GPIO_WritePin(GPIOC, COL_1_Pin | COL_2_Pin | COL_3_Pin | COL_4_Pin, GPIO_PIN_RESET);

        // 4. Turn ON just the current column
        switch (current_col)
        {
            case 0: HAL_GPIO_WritePin(GPIOC, COL_1_Pin, GPIO_PIN_SET); break;
            case 1: HAL_GPIO_WritePin(GPIOC, COL_2_Pin, GPIO_PIN_SET); break;
            case 2: HAL_GPIO_WritePin(GPIOC, COL_3_Pin, GPIO_PIN_SET); break;
            case 3: HAL_GPIO_WritePin(GPIOC, COL_4_Pin, GPIO_PIN_SET); break;
        }

        // 5. Read the rows to see if 3.3V made it through a pushed button
        uint8_t row_pressed = 255; // 255 acts as our "nothing pressed" flag

        if (HAL_GPIO_ReadPin(GPIOC, ROW_1_Pin) == GPIO_PIN_SET) row_pressed = 0;
        else if (HAL_GPIO_ReadPin(GPIOC, ROW_2_Pin) == GPIO_PIN_SET) row_pressed = 1;
        else if (HAL_GPIO_ReadPin(GPIOC, ROW_3_Pin) == GPIO_PIN_SET) row_pressed = 2;
        else if (HAL_GPIO_ReadPin(GPIOC, ROW_4_Pin) == GPIO_PIN_SET) row_pressed = 3;

        // Static variables remember their state across interrupts
        static char last_key = '\0';
        static uint8_t empty_scans = 0;

        // 6. If a row was HIGH, we found a button press!
        if (row_pressed != 255)
        {
        	char pressed_key = keypad_map[row_pressed][current_col];
        	empty_scans = 0;	// Reset our empty counter since we found a press

        	// Only send the key to the main loop if it's a NEW press
        	if (pressed_key != last_key)
        	{
        		new_key = pressed_key;
        		last_key = pressed_key;
        	}
        }
        else
        {
        	empty_scans++;
        	// If we checked all 4 columns (20ms) and found no voltage,
        	// the user definitely let go of the button
        	if (empty_scans >= 4)
        	{
        		last_key = '\0';	// Clear the memory so they can press it again
        	}
        }

        // 7. Move to the next column for the next 5ms cycle
        current_col++;
        if (current_col > 3)
        {
            current_col = 0;
        }
    }
}

// This function fires automatically the microsecond DMA finishes reading the IMU
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        // Only check for movement if the vault is actually locked
        if (currentState == STATE_ARMED)
        {
            int16_t z_axis = (i2c_rx_buffer[4] << 8) | i2c_rx_buffer[5];

            // The sensor debounce filter
            static uint8_t motion_counter = 0;

            // Check if the board is being picked up
            if (z_axis < 12000 || z_axis > 20000)
            {
            	motion_counter++;

            	// Only trigger if we get 5 bad readings in a row
            	if (motion_counter >= 5)
            	{
            		currentState = STATE_ALARM;
            		motion_counter = 0;		// Reset the counter
            	}
            }
            else
            {
            	// Good reading resets the counter
            	motion_counter = 0;
            }
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
