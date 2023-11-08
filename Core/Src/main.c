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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "utils.h"
#include "UartRingbuffer.h"
#include "mode.c"
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MODES_LENGTH 8
#define UART_TIMEOUT 10
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
//UART_HandleTypeDef huart6;
/* USER CODE BEGIN PV */

uint32_t PINS[] = {GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15};

struct Mode modes[] = {
		  { { LED_GREEN, LED_YELLOW}, 2, 1000, 0},
		  { { LED_GREEN, LED_RED}, 2, 200, 0},
		  { { LED_YELLOW, LED_RED}, 2, 200, 0},
		  { { LED_NONE, LED_YELLOW }, 2, 200, 0},
		  { { LED_NONE, LED_NONE }, 2, 200, 0},
		  { { LED_NONE, LED_NONE }, 2, 200, 0},
		  { { LED_NONE, LED_NONE}, 2, 200, 0},
		  { { LED_NONE, LED_NONE }, 2, 200, 0}
};

int cur_mode_index = 0;
int modes_size = 4;
int index_last_changed_mode = 3;

bool expecting_delay_input = false;
bool interrupts_mode = false;

int new_mode_length = 0;
int buffer_mode[LENGTH] = {0,};

char recieved_char;
char cmd[UART_BUFFER_SIZE] = {0,};
int index_char = 0;

const char NEWLINE_CHAR = '\r';
const char BACKSPACE_CHAR = '\177';

const int FAST_SPEED = 200;
const int MEDIUM_SPEED = 500;
const int SLOW_SPEED = 1000;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int parse_mode(const char *s) {
	int i = 0;

	while (*s) {
		if  (i >= LENGTH) {
			return -1;
		}

		switch (*s++) {
			case 'g':
				buffer_mode[i] = LED_GREEN;
				break;
			case 'r':
				buffer_mode[i] = LED_RED;
				break;
			case 'y':
				buffer_mode[i] = LED_YELLOW;
				break;
			case 'n':
				buffer_mode[i] = LED_NONE;
				break;
			default:
				return -1;
		}

		i++;
	}
	return (i > 1) ? i : -1;
}

void clear_cmd() {
	for (int j = 0; j < index_char; j++) {
		cmd[j] = 0;
	}

	index_char = 0;
}

void handle_data() {
	// Echo
	print(&recieved_char);

	if (recieved_char == NEWLINE_CHAR) {
		print("\n");

		// If some command is present, try to execute.
		// If not, just go to next line without an error message.
		if (index_char != 0) {
			execute_command();
		}

		clear_cmd();
		return;
	}

	if (recieved_char == BACKSPACE_CHAR) {
		cmd[--index_char] = 0;
		return;
	}

	if (index_char >= UART_BUFFER_SIZE) {
		print("\r\nBuffer overflow!\r\n");
		clear_cmd();
		return;
	}

	cmd[index_char++] = recieved_char;
}



int add_mode(int code[], int size, int delay) {
	index_last_changed_mode++;

	if (index_last_changed_mode >= MODES_LENGTH) {
		// Return to first mode after first 4 constant modes
		index_last_changed_mode = 4;
	}

	struct Mode new_mode = {
			.size = size,
			.delay = delay,
			.current_code_index = 0
	};

	for (int i = 0; i < size; i++) {
		new_mode.code[i] = code[i];
	}

	modes_size++;

	modes[index_last_changed_mode] = new_mode;

	return index_last_changed_mode;
}

int handle_delay_input() {
	if (strcmp(cmd, "fast") == 0 || strcmp(cmd, "f") == 0) {
		 return FAST_SPEED;
	}

	if (strcmp(cmd, "medium") == 0 || strcmp(cmd, "m") == 0) {
		return MEDIUM_SPEED;
	}

	if (strcmp(cmd, "slow") == 0 || strcmp(cmd, "s") == 0) {
		return SLOW_SPEED;
	}

	return -1;
}


void execute_command() {
	if (expecting_delay_input) {
		int delay = handle_delay_input();

		if (delay == -1) {
			print("Invalid delay. Try again: ");
			return;
		}

		add_mode(buffer_mode, new_mode_length, delay);

		const char* fmt = "OK\r\nNumber of new mode: %i\r\n";
		char line[strlen(fmt)];
		sprintf(line, fmt, index_last_changed_mode + 1);
		print(line);

		expecting_delay_input = false;

		return;
	}


	if (strcmp(cmd, "set interrupts on") == 0) {
		interrupts_mode = true;
		print("OK\r\n");
		__HAL_UART_ENABLE_IT(&huart6, UART_IT_TXE);
		__HAL_UART_ENABLE_IT(&huart6, UART_IT_RXNE);
		return;
	}

	if (strcmp(cmd, "set interrupts off") == 0) {
		interrupts_mode = false;
		print("OK\r\n");
		__HAL_UART_DISABLE_IT(&huart6, UART_IT_TXE);
		__HAL_UART_DISABLE_IT(&huart6, UART_IT_RXNE);
		return;
	}

	if (strstr(cmd, "set ") == cmd && strlen(cmd) > 4) {
		int p = atoi(&cmd[4]);

		if (p <= modes_size && p > 0){
			cur_mode_index = p - 1;
			print("OK\r\n");
		} else {
			print("This mode does not exist\r\n");
		}

		return;
	}

	if (strstr(cmd, "new ") == cmd) {
		new_mode_length = parse_mode(&cmd[4]);

		if (new_mode_length == -1) {
			print("Invalid parameter\r\n");
			return;
		}

		print("Input delay: fast/f (200 ms), medium/m (500 ms), slow (1000 ms): ");
		expecting_delay_input = true;

		return;
	}

	print("Invalid command (new xyz, set x, set interrupts on/off)\r\n");
}

bool activate_mode(struct Mode* current_mode) {
	bool mode_switched = false;

	bool btn_state = is_btn_press();

	int led = current_mode->code[current_mode->current_code_index];

	if (led != LED_NONE) {
		HAL_GPIO_WritePin(GPIOD, PINS[led], GPIO_PIN_SET);
	}

	int start_time = HAL_GetTick();

	while (HAL_GetTick() < start_time + current_mode->delay) {
		if (!mode_switched) {
			bool i = is_btn_press();
			mode_switched = !i && btn_state;
			btn_state = i;
		}

		if (interrupts_mode) {
			while (is_data_available()) {
				recieved_char = uart_read();
				handle_data();
			}
		} else {
			if (HAL_UART_Receive(&huart6, &recieved_char, 1, 50) == HAL_OK) {
				handle_data();
			}
		}
	}

	current_mode->current_code_index++;
	if (current_mode->current_code_index >= current_mode->size) {
		current_mode->current_code_index = 0;
	}

	if (led != LED_NONE) {
		HAL_GPIO_WritePin(GPIOD, PINS[led], GPIO_PIN_RESET);
	}

	return mode_switched;
}


void print(const char * content) {
	if (interrupts_mode) {
		uart_sendstring(content);
	} else {
		HAL_UART_Transmit(&huart6, (void *) content, strlen(content), UART_TIMEOUT);
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
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
  Ringbuf_init ();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  bool switched = activate_mode(&modes[cur_mode_index]);

	  if (switched) {
	  		cur_mode_index++;
	  		if (cur_mode_index >= modes_size) {
	  			cur_mode_index = 0;
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
