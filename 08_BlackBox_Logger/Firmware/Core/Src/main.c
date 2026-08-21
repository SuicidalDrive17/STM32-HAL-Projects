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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
	STATE_IDLE,				// Waiting for a START command from PC
	STATE_READ_TIME,		// Reading time from RTC via I2C
	STATE_TIME_PENDING,		// Resting while DMA does the hardware work
	STATE_READ_SENSOR,		// Reading data from the environmental sensor
	STATE_SENSOR_PENDING, 	// Waiting for IMU data DMA
	STATE_SEND_UART,		// Formatting and triggering UART transmission
	STATE_TX_PENDING,		// Waiting for UART DMA to finish sending
	STATE_ERROR				// Handling system faults
} SystemState_t;

SystemState_t currentState = STATE_IDLE;
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

SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */
uint8_t rx_data;
uint8_t rx_buffer[1];	// Buffer to store a 1-character command (e.g., 'S' for Start)

#define DS1307_ADDR (0x68 << 1)	// DS1307 I2C address (0x68 shifted left by 1 bit for the HAL library)

// Buffers and variables for timekeeping
uint8_t rtc_rx_buffer[3];
uint8_t time_sec, time_min, time_hour;

// MPU9250 Address with AD0 pulled to 3.3V (0x69 shifted left by 1)
#define MPU9250_ADDR (0x69 << 1)

// Array to hold 6 bytes of raw accelerometer data (X, Y, Z axes, 2 bytes each)
uint8_t imu_rx_buffer[6];

// Variables to hold the stitched 16-bit accelerometer data
int16_t accel_x, accel_y, accel_z;

// Buffer to hold our formatted text string
char tx_buffer[100];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI2_Init(void);
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
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_SPI2_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  // Arm the UART DMA to listen for 1 byte in the background
  HAL_UART_Receive_IT(&huart2, (uint8_t *)&rx_data, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // Main state machine execution
	  switch (currentState)
	  {
	  	  case STATE_IDLE:
	  		  // System is resting, waiting for UART DMA interrupt to trigger a state change
	  		  break;
	  	  case STATE_READ_TIME:
	  		  // Trigger non-blocking I2C read for RTC
	  		  // Ask DMA to read 3 bytes starting from register 0x00
	  		  if (HAL_I2C_Mem_Read_DMA(&hi2c1, DS1307_ADDR, 0x00, 1, rtc_rx_buffer, 3) == HAL_OK)
	  		  {
	  			  // Hardware is busy
	  			  currentState = STATE_TIME_PENDING;
	  		  }
	  		  else
	  		  {
	  			  currentState = STATE_ERROR;
	  		  }
	  		  break;

	  	  case STATE_TIME_PENDING:
	  		  // The CPU does absolutely nothing here
	  		  // We just wait for the I2C DMA hardware interrupt to tap us on the shoulder
	  		  break;
	  	  case STATE_READ_SENSOR:
			  // Trigger non-blocking I2C read for sensor data
	  		  // We ask DMA to grab 6 consecutive bytes (X_High, X_Low, Y_High, Y_Low, Z_High, Z_Low)
	  		  if (HAL_I2C_Mem_Read_DMA(&hi2c1, MPU9250_ADDR, 0x3B, 1, imu_rx_buffer, 6) == HAL_OK)
	  		  {
	  			  currentState = STATE_SENSOR_PENDING;
	  		  }
	  		  else
	  		  {
	  			  currentState = STATE_ERROR;
	  		  }
	  		  break;

	  	  case STATE_SENSOR_PENDING:
	  		  // CPU rests here while DMA pulls the 6 bytes from the IMU
	  		  break;
	  	  case STATE_SEND_UART:
	  		  // Format data into human-readable string
	  		  // %02d ensures single digits get a leading zero (e.g., "09" instead of "9")
	  		  snprintf(tx_buffer, sizeof(tx_buffer), "Time: %02d:%02d:%02d | X:%d, Y:%d, Z:%d\r\n",
	  				  time_hour, time_min, time_sec, accel_x, accel_y, accel_z);

	  		  // Ask the DMA to send it over USB cable to the PC
	  		  if (HAL_UART_Transmit_DMA(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer)) == HAL_OK)
	  		  {
	  			  // Shift to pending state so the CPU doesn't freeze while sending
	  			  currentState = STATE_TX_PENDING;
	  		  }
	  		  else
	  		  {
	  			  currentState = STATE_ERROR;
	  		  }
	  		  break;
	  	  case STATE_TX_PENDING:

	  		  break;
	  	  case STATE_ERROR:
	  		  // Turn on red LED, halt system or wait for watchdog reset
	  		  // Blast an error message to the PC so we know it failed!
			  sprintf(tx_buffer, "HARDWARE FAULT: I2C failed to read.\r\n");
			  HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, strlen(tx_buffer), 1000);
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
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

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
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
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
  HAL_GPIO_WritePin(GPIOA, SD_CS_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SD_CS_Pin LD2_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// This callback fires automatically in the background when a byte is received
// ---------------------------------------------------------
// UART INTERRUPT CALLBACK (Wakes system up)
// ---------------------------------------------------------
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2)
	{
		// Check which character was sent from the PC
		if (rx_data == 'S')
		{
			// 'S' for Start logging: transition to the first active state
			currentState = STATE_READ_TIME;
		}
		else if (rx_data == 'X')
		{
			// 'X' for Stop logging: force the system back to idle
			currentState = STATE_IDLE;
		}

		// Re-arm the listener for the NEXT command you type
		HAL_UART_Receive_IT(&huart2, (uint8_t *)&rx_data, 1);
	}
}

// ---------------------------------------------------------
// I2C INTERRUPT CALLBACK (Fires when time is fetched)
// ---------------------------------------------------------
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == I2C1)
	{
		// Did the DMA just finish fetching the time?
		if (currentState == STATE_TIME_PENDING)
		{
			// Convert the raw BCD (Binary-Coded Decimal) data into human-readable numbers
			time_sec = ((rtc_rx_buffer[0] >> 4) * 10) + (rtc_rx_buffer[0] & 0x0F);
			time_min = ((rtc_rx_buffer[1] >> 4) * 10) + (rtc_rx_buffer[1] & 0x0F);
			time_hour = ((rtc_rx_buffer[2] >> 4) * 10) + (rtc_rx_buffer[2] & 0x0F);

			currentState = STATE_READ_SENSOR;
		}
		// Or did the DMA just finish fetching the IMU data?
		else if (currentState == STATE_SENSOR_PENDING)
		{
			// Stitch the High and Low bytes together using Bitwise Left Shift (<<) and OR (|)
			accel_x = (int16_t)((imu_rx_buffer[0] << 8) | imu_rx_buffer[1]);
			accel_y = (int16_t)((imu_rx_buffer[2] << 8) | imu_rx_buffer[3]);
			accel_z = (int16_t)((imu_rx_buffer[4] << 8) | imu_rx_buffer[5]);

			// Data fully processed
			currentState = STATE_SEND_UART;
		}
	}
}

// ---------------------------------------------------------
// UART TX INTERRUPT CALLBACK (Fires when PC receives data)
// ---------------------------------------------------------
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (currentState == STATE_TX_PENDING)
        {
            // The text was sent! Loop back to grab the next reading.
            currentState = STATE_READ_TIME;
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
