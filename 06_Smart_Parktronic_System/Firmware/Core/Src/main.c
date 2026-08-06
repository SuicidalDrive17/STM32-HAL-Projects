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
// __attribute__((packed)) doesn't allow to add empty bytes protecting frame from becoming bigger
typedef struct __attribute__((packed))
{
	uint8_t start_byte;	// Frame start marker (e.g. 0xAA)
	uint8_t msg_type;	// 0x01 - Distance
	float distance;		// Raw 4 bytes of floating-point data
	uint8_t checksum;	// XOR checksum for data integrity
} TelemetryFrame;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

/* USER CODE BEGIN PV */
TelemetryFrame tx_frame;	// Initialization of the payload struct

uint8_t rx_buffer[50];

// Sensor logic variables
uint32_t raw_time = 0;
float distance_cm = 0.0f;	// Using f at the end compiler knows it's 32-bit not 64-bit double
uint8_t sensor_ready = 0;

// Host (Python) State Variables
uint8_t host_led_state = 0;
uint8_t host_buzzer_state = 0;

uint8_t system_on = 0;	// 0 = OFF, 1 = ON
char lcd_buffer[16];	// Text buffer for formatting LCD numbers

// Advanced Non-Blocking Button Variables
uint8_t button_state = 1;	// Current state (1 = unpressed cause we sue pull up)
uint8_t last_button_state = 1;
uint32_t last_debounce_time = 0;	// Timestamp of the last physical bounce
uint32_t debounce_delay = 50;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void LCD_Init(void);
void LCD_Command(uint8_t cmd);
void LCD_Char(char data);
void LCD_String(char *str);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_SendHalfByte(uint8_t val);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == USART2)
	{
		// Null-terminate the received data to treat it as standard C string
		rx_buffer[Size] = '\0';

		// Parse the incoming command: format <L,B> (e.g., <1,2>)
		if (rx_buffer[0] == '<' && strchr((char*)rx_buffer, '>') != NULL)
		{
			host_led_state = rx_buffer[1] - '0';	// This stands for '1' - '0' (example)
			host_buzzer_state = rx_buffer[3] - '0';	// This stands for '2' - '0' (example)
		}

		// Restart the DMA listening for the next incoming command
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buffer, 50);	// When 50 bytes received or idle state DMA to RxEventCallback function
		__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);	// Disable Half-Transfer interrupt
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
  MX_DMA_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  // Start the hardware Timer 3 for Input Capture (Echo)
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);

  // Start listening to Python commands via DMA + IDLE line detection
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buffer, 50);
  __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);

  LCD_Init();
  LCD_SetCursor(0, 0);
  LCD_String("System OFF");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // 0. Hardware button logic
	  uint8_t current_reading = HAL_GPIO_ReadPin(MAIN_BTN_GPIO_Port, MAIN_BTN_Pin);

	  // Non-blocking buzzer beeping logic, led blinking logic and button logic
	  uint32_t current_time = HAL_GetTick();

	  if (current_reading != last_button_state)
	  {
		  last_debounce_time = current_time;	// Reset the bouncing timer
	  }

	  // If the state has been stable longer than the debounce delay
	  if ((current_time - last_debounce_time) > debounce_delay)
	  {
		  // If the button state has actually changed to a new stable state
		  if (current_reading != button_state)
		  {
			  button_state = current_reading;

			  // Only trigger the action on the FALLING EDGE (when button is pressed down)
			  // Since we use Pull-Up, pressed down means GPIO_PIN_RESET (0)
			  if (button_state == 0)
			  {
				  system_on = !system_on;	// System state toggled

				  if (system_on == 0)
				  {
					  // Shutdown sequence
					  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
					  HAL_GPIO_WritePin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin, GPIO_PIN_RESET);
					  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
					  HAL_GPIO_WritePin(BUZZER_ACTIVE_GPIO_Port, BUZZER_ACTIVE_Pin, GPIO_PIN_RESET);

					  // Display fading
					  LCD_Command(0x01);	// Clear display
					  HAL_Delay(2);
					  LCD_SetCursor(0, 0);
					  LCD_String("System OFF");
				  }
				  else
				  {
					  // Boot sequence (clear display before measurements)
					  LCD_Command(0x01);	// Clear display
					  HAL_Delay(2);
				  }
			  }
		  }
	  }

	  last_button_state = current_reading;

	  if (system_on == 1)
	  {
		  // 1. Sensor Trigger (Non-Blocking execution)
		  // 10 microsecond HIGH pulse to the TRIG pin
		  HAL_GPIO_WritePin(SENSOR_TRIG_GPIO_Port, SENSOR_TRIG_Pin, GPIO_PIN_SET);
		  for (volatile int i = 0; i < 800; i++) {}	// Tiny hardware delay for the trigger pulse
		  HAL_GPIO_WritePin(SENSOR_TRIG_GPIO_Port, SENSOR_TRIG_Pin, GPIO_PIN_RESET);

		  // 2. Calculate distance
		  // Convert raw timer ticks (us) into cm
		  // Sound travels 1 cm in approx 29 us, round trip = 58
		  if (raw_time > 0)
		  {
			  distance_cm = (float)raw_time / 58.0f;
		  }

		  // 3. Transmit binary frame to python
		  tx_frame.start_byte = 0xAA;
		  tx_frame.msg_type = 0x01;
		  tx_frame.distance = distance_cm;

		  // Calculate XOR checksum for data integrity
		  uint8_t *ptr = (uint8_t*)&tx_frame;
		  tx_frame.checksum = ptr[0] ^ ptr[1] ^ ptr[2] ^ ptr[3] ^ ptr[4] ^ ptr[5];

		  // Transmit 7 bytes of the struct via UART
		  HAL_UART_Transmit(&huart2, (uint8_t*)&tx_frame, 7, 100); // Only 7 bytes so the easiest way is to send it using interrupts

		  // 4. LCD update
		  // Split the float into 2 safe int values
		  uint32_t dist_whole = (uint32_t)distance_cm;
		  uint32_t dist_fraction = (uint32_t)(distance_cm * 10.0f) % 10;

		  // Safe snprintf and lightweight %lu formats (long unsigned int)
		  snprintf(lcd_buffer, sizeof(lcd_buffer), "Dist: %3lu.%1lu cm ", dist_whole, dist_fraction);

		  LCD_SetCursor(0, 0);
		  LCD_String(lcd_buffer);

		  LCD_SetCursor(1, 0);
		  if (host_led_state == 1) LCD_String("Status: CLEAR  ");
		  else if (host_led_state == 2) LCD_String("Status: WARNING");
		  else if (host_led_state == 3 || host_led_state == 4) LCD_String("Status: ALARM! ");

		  // 5. Execute host commands (Leds & Buzzer)
		  // Reset all LEDs first
		  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin, GPIO_PIN_RESET);
		  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);

		  // State requested by python
		  if (host_led_state == 1) HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
		  if (host_led_state == 2) HAL_GPIO_WritePin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin, GPIO_PIN_SET);
		  if (host_led_state == 3) HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
		  if (host_led_state == 4)
		  {
			  if ((current_time % 600) < 300)
			  {
				  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
			  }
			  else
			  {
				  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
			  }
		  }


		  if (host_buzzer_state == 0)
		  {
			  // State 0: Silent
			  HAL_GPIO_WritePin(BUZZER_ACTIVE_GPIO_Port, BUZZER_ACTIVE_Pin, GPIO_PIN_RESET);

		  }
		  else if (host_buzzer_state == 1)
		  {
			  // State 1: Slow beep (Toggle every 400 ms)
			  if ((current_time % 800) < 400)
			  {
				  HAL_GPIO_WritePin(BUZZER_ACTIVE_GPIO_Port, BUZZER_ACTIVE_Pin, GPIO_PIN_SET);
			  }
			  else
			  {
				  HAL_GPIO_WritePin(BUZZER_ACTIVE_GPIO_Port, BUZZER_ACTIVE_Pin, GPIO_PIN_RESET);
			  }
		  }
		  else if (host_buzzer_state == 2)
		  {
			  // State 2: Fast beep (Toggle every 150 ms)
			  if ((current_time % 300) < 150)
			  {
				  HAL_GPIO_WritePin(BUZZER_ACTIVE_GPIO_Port, BUZZER_ACTIVE_Pin, GPIO_PIN_SET);
			  }
			  else
			  {
				  HAL_GPIO_WritePin(BUZZER_ACTIVE_GPIO_Port, BUZZER_ACTIVE_Pin, GPIO_PIN_RESET);
			  }
		  }
		  else if (host_buzzer_state == 3)
		  {
			  // State 3: Continuous emergency alarm
			  HAL_GPIO_WritePin(BUZZER_ACTIVE_GPIO_Port, BUZZER_ACTIVE_Pin, GPIO_PIN_SET);
		  }
	  }

	  HAL_Delay(50);
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
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 80-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

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
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

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
  HAL_GPIO_WritePin(GPIOC, LED_GREEN_Pin|LED_YELLOW_Pin|LED_RED_Pin|BUZZER_ACTIVE_Pin
                          |LCD_RS_Pin|LCD_EN_Pin|LCD_D7_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|SENSOR_TRIG_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_D4_Pin|LCD_D5_Pin|LCD_D6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_GREEN_Pin LED_YELLOW_Pin LED_RED_Pin BUZZER_ACTIVE_Pin
                           LCD_RS_Pin LCD_EN_Pin LCD_D7_Pin */
  GPIO_InitStruct.Pin = LED_GREEN_Pin|LED_YELLOW_Pin|LED_RED_Pin|BUZZER_ACTIVE_Pin
                          |LCD_RS_Pin|LCD_EN_Pin|LCD_D7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin SENSOR_TRIG_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|SENSOR_TRIG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_D4_Pin LCD_D5_Pin LCD_D6_Pin */
  GPIO_InitStruct.Pin = LCD_D4_Pin|LCD_D5_Pin|LCD_D6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : MAIN_BTN_Pin */
  GPIO_InitStruct.Pin = MAIN_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(MAIN_BTN_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// Local variables for capturing timer states
uint32_t ic_val1 = 0;
uint32_t ic_val2 = 0;
uint8_t is_first_captured = 0;

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
	{
		if (is_first_captured == 0)	// Rising edge detected
		{
			ic_val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
			is_first_captured = 1;

			// Reconfigure the hardware to look for the falling edge now
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
		}
		else	// Falling edge detected
		{
			ic_val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

			// Calculate the difference  avoiding 16-bit timer overflow issues
			if (ic_val2 > ic_val1)
			{
				raw_time = ic_val2 - ic_val1;
			}
			else if (ic_val1 > ic_val2)
			{
				raw_time = (0xFFFF - ic_val1) + ic_val2;
			}

			is_first_captured = 0;

			// Reset polarity to wait for the next rising edge
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
		}
	}
}

// Core function to send 4-bits of data to the LCD hardware pins
void LCD_SendHalfByte(uint8_t val)
{
    HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, (val >> 0) & 0x01);
    HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, (val >> 1) & 0x01);
    HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, (val >> 2) & 0x01);
    HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, (val >> 3) & 0x01);

    // Hardware Enable Pulse (Strobe) to latch the data inside LCD memory
    HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_SET);
    for(volatile int i = 0; i < 200; i++); // Microsecond hardware delay
    HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_RESET);
    for(volatile int i = 0; i < 200; i++);
}

void LCD_Command(uint8_t cmd)
{
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET); // RS Low = Command mode
    LCD_SendHalfByte(cmd >> 4);   // Send high nibble
    LCD_SendHalfByte(cmd & 0x0F);  // Send low nibble
    HAL_Delay(2); // Command execution delay
}

void LCD_Char(char data)
{
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET); // RS High = Data mode
    LCD_SendHalfByte(data >> 4);   // Send high nibble
    LCD_SendHalfByte(data & 0x0F);  // Send low nibble
    for(volatile int i = 0; i < 500; i++); // Character generation delay
}

void LCD_String(char *str)
{
    while(*str)
    {
        LCD_Char(*str++);
    }
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t mask = (row == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_Command(mask);
}

void LCD_Init(void)
{
    HAL_Delay(50); // Wait for LCD power stabilization
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_RESET);

    // Hardcoded internal state machine reset for 4-bit mode conversion
    LCD_SendHalfByte(0x03);
    HAL_Delay(5);
    LCD_SendHalfByte(0x03);
    HAL_Delay(1);
    LCD_SendHalfByte(0x03);
    HAL_Delay(1);
    LCD_SendHalfByte(0x02); // Officially switch HD44780 controller to 4-bit mode

    // Display configuration commands
    LCD_Command(0x28); // 4-bit interface, 2 lines, 5x8 font matrix
    LCD_Command(0x0C); // Display ON, Cursor OFF, Blink OFF
    LCD_Command(0x06); // Auto-increment cursor, shift off
    LCD_Command(0x01); // Clear display memory
    HAL_Delay(2);
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
