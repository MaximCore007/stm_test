/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define USART_BUFFER_SIZE 512U
#define LED_PWD_FREQ 	200U
#define LED_PWM_TIM_CHANNEL TIM_CHANNEL_1

#define I2C_BUFFER_SIZE 128U
#define I2C_SLAVE_ADDRESS 0xA0
#define I2C_MEM_ADDRESS 0x00
#define I2C_MEM_ADD_SIZE 2U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint8_t usart_buffer[USART_BUFFER_SIZE];
static uint8_t i2c_buffer[I2C_BUFFER_SIZE];

enum COMMAND_N_LIST {
	COMMAND_LED_U_OFF = 1,
	COMMAND_LED_U_ON,
	COMMAND_LED_PWM_OFF,
	COMMAND_LED_PWM_ON,
	COMMAND_I2C_READ,
	COMMAND_END,
};

const static char *COMMAND_LIST[] = {
		[COMMAND_LED_U_OFF] = "led1 off",
		[COMMAND_LED_U_ON] =  "led1 on",
		[COMMAND_LED_PWM_OFF] =  "led2 off",
		[COMMAND_LED_PWM_ON] =  "led2 on",
		[COMMAND_I2C_READ] = "read i2c",
};

static int duty_percent = 30;

static struct I2C_MEM_PARAM {
	uint16_t slave_addr;
	uint16_t mem_addr;
	uint16_t mem_addr_size;
	uint16_t buffer_size;	// data size
	uint8_t *buffer;
} i2c_dev;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void I2C_SlaveDeviceInit(void)
{
	i2c_dev.buffer = i2c_buffer;
	i2c_dev.buffer_size = I2C_BUFFER_SIZE;
	i2c_dev.slave_addr = I2C_SLAVE_ADDRESS;
	i2c_dev.mem_addr = I2C_MEM_ADDRESS;
	i2c_dev.mem_addr_size = I2C_MEM_ADD_SIZE;
}

static void USART_SendMessage(uint8_t *buffer, uint16_t size)
{
	if (size > USART_BUFFER_SIZE) {
		size = USART_BUFFER_SIZE;
	}
	memcpy(usart_buffer, buffer, size);
	HAL_UART_Transmit(&huart1, usart_buffer, size, 100);
}

static enum COMMAND_N_LIST Command_receive(void)
{
	HAL_UART_Receive(&huart1, usart_buffer, USART_BUFFER_SIZE, 100);

	for (enum COMMAND_N_LIST i = 1; i < COMMAND_END; i++) {
		if (strcmp(COMMAND_LIST[i], (char *)usart_buffer) == 0) {
			return i;
		}
	}
	return 0;
}

static void LED_PWM_Init(void)
{
	uint32_t APBfreq = HAL_RCC_GetPCLK1Freq();
    APBfreq *= (RCC->CFGR & RCC_CFGR_PPRE1) == 0 ? 1 : 2;

    uint32_t TIMfreq = APBfreq;
	htim3.Init.Prescaler = 0;
    while ((TIMfreq / LED_PWD_FREQ) > 65535) {
		htim3.Init.Prescaler += 2;
		TIMfreq = APBfreq / htim3.Init.Prescaler;
    }
    if (htim3.Init.Prescaler > 0)
		htim3.Init.Prescaler -= 1;

	htim3.Init.Period = (APBfreq / LED_PWD_FREQ) - 1;

	htim3.Instance = TIM3;
	if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
	{
		Error_Handler();
	}
}

static void Command_handler(enum COMMAND_N_LIST command)
{
	switch (command)
	{
	case COMMAND_LED_U_OFF:
		HAL_GPIO_WritePin(LED_UART_GPIO_Port, LED_UART_Pin, GPIO_PIN_SET);
		break;
	case COMMAND_LED_U_ON:
		HAL_GPIO_WritePin(LED_UART_GPIO_Port, LED_UART_Pin, GPIO_PIN_RESET);
		break;
	case COMMAND_LED_PWM_OFF:
		HAL_TIMEx_PWMN_Stop(&htim3, LED_PWM_TIM_CHANNEL);
		break;
	case COMMAND_LED_PWM_ON:
		int ARR = __HAL_TIM_GET_AUTORELOAD(&htim3);
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (duty_percent * ARR / 100));
		HAL_TIMEx_PWMN_Start(&htim3, LED_PWM_TIM_CHANNEL);
		break;
	case COMMAND_I2C_READ:
		HAL_I2C_Mem_Read(&hi2c1, i2c_dev.slave_addr, i2c_dev.mem_addr, i2c_dev.mem_addr_size, i2c_dev.buffer, i2c_dev.buffer_size, 100);
		USART_SendMessage(i2c_dev.buffer, i2c_dev.buffer_size);
		break;
	default:
		break;
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
  MX_I2C1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  static enum COMMAND_N_LIST cmd = 0;

  I2C_SlaveDeviceInit();
  LED_PWM_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  cmd = Command_receive();
	  if (cmd != 0) {
		  Command_handler(cmd);
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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
