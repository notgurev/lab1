/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "UartRingbuffer.h"
#include "mode.c"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

struct Mode MODES[] = {
		  {LED_GREEN, 10},
		  {LED_GREEN, 40},
		  {LED_GREEN, 100},
		  {LED_YELLOW, 10},
		  {LED_YELLOW, 40},
		  {LED_YELLOW, 100},
		  {LED_RED, 10},
		  {LED_RED, 40},
		  {LED_RED, 100}
};

// Buffer for mode being edited
struct Mode buffer_mode = {LED_GREEN, 0};

int input_index = 0;

bool is_editing_mode = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void set_pin(int led, int brightness){
	htim4.Instance->CCR2 = 0;
	htim4.Instance->CCR3 = 0;
	htim4.Instance->CCR4 = 0;

	switch (led) {
		case 0:
			htim4.Instance->CCR2 = 10 * brightness;
			break;
		case 1:
			htim4.Instance->CCR3 = 10 * brightness;
			break;
		case 2:
			htim4.Instance->CCR4 = 10 * brightness;
			break;
	}
}

void print(const char * content) {
	uart_sendstring(content);
}

void print_mode_description(struct Mode mode, int index){
	if (index >= 0){
		print("Editing mode ");
		char str_index[1];
		sprintf(str_index, "%i", index + 1);
		print(str_index);
		print(" :");
	} else {
		print("Mode: ");
	}

	switch (mode.led) {
		case 0: print("green, "); break;
		case 1: print("yellow, "); break;
		case 2: print("red, "); break;
	}

	char mode_brightness[3];
	sprintf(mode_brightness, "%i", mode.brightness);
	print(mode_brightness);
	print("% brightness\n\r");
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
  MX_USART6_UART_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  Ringbuf_init ();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
  __HAL_UART_ENABLE_IT(&huart6, UART_IT_TXE);
  __HAL_UART_ENABLE_IT(&huart6, UART_IT_RXNE);
  set_pin(-1, 0);

  bool changed = false; // whether mode was modified in editing mode

  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	// Wait for available data
	if (!is_data_available()) {
		continue;
	}

	char received_char = uart_read();

	if (is_editing_mode) {
		if (received_char == '\r') {
			is_editing_mode = false;

			if (changed) {
				MODES[input_index].led = buffer_mode.led;
				MODES[input_index].brightness = buffer_mode.brightness;
				print("Mode settings saved\n\r");
			}

			changed = false;

			print("\n\r== Editing mode disabled ==\n\r");

			continue;
		}

		changed = true;

		int i = atoi(&received_char) - 1;
		if (i >= 0) {
			input_index = i;
			buffer_mode.led = MODES[input_index].led;
			buffer_mode.brightness = MODES[input_index].brightness;
		} else switch (received_char) {
			case 'a' :
				buffer_mode.led = LED_GREEN;
				print("Changed color to green\r\n");
				break;
			case 'b' :
				buffer_mode.led = LED_YELLOW;
				print("Changed color to yellow\r\n");
				break;
			case 'c' :
				buffer_mode.led = LED_RED;
				print("Changed color to red\r\n");
				break;
			case '-' :
				buffer_mode.brightness = buffer_mode.brightness >= 10 ? buffer_mode.brightness - 10 : 0;
				print("Decreased brightness\r\n");
				break;
			case '+' :
				buffer_mode.brightness = buffer_mode.brightness <= 90 ? buffer_mode.brightness + 10 : 100;
				print("Increased brightness\r\n");
				break;
			default: // wrong character, nothing actually changed
				changed = false;
				break;
		}

		if (changed) {
			print_mode_description(buffer_mode, input_index);
		}

		continue;
	}

	if (received_char == '\r') {
		is_editing_mode = true;
		print("\n\r== Editing mode enabled ==\n\r");
		print("Colors: a - green, b - yellow, c - red\n\r");
		print("Brightness: + to increase, - to decrease\n\r");
		print("Select mode by: 1-9\n\r");
		continue;
	}

	int mode_num = atoi(&received_char); // starts from 1

	int mode_index = mode_num - 1;

	if (mode_index != -2) {
		if (mode_index != -1) {
			set_pin(MODES[mode_index].led, MODES[mode_index].brightness);
			print_mode_description(MODES[mode_index], -1);
		} else {
			set_pin(-1, 0);
			print("Disabled all diodes\n\r");
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
//  __disable_irq();
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
