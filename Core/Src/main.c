/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "ring_buffer.h"
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

const char OK_MESSAGE[]= "OK";
const char WRONG_COMMAND[] = "Wrong command";

// Buffer for commands
uint8_t cmd[UART_BUFFER_SIZE];

uint32_t last_pressed_time = 0;
uint8_t adding_mode = 3;
uint8_t cur_mode_code = 0;

bool is_interrupt_mode = false;

char last_received_char;
const char * content_all;

uint8_t remaining_timeouts_input = 0;
uint8_t CONST_MODES_COUNT = 4;
uint8_t current_max_mode = 3;
#define MAX_MODES_COUNT 8

enum LED {
    LED_NO_ONE = 0,
    LED_RED = 1,
    LED_GREEN = 2,
    LED_YELLOW = 3,
};

enum TIMEOUTS {
	SLOW = 500,
	MEDIUM = 250,
	FAST = 100,
};

struct LightState {
    enum LED color;
    enum TIMEOUTS timeout;
};

struct LEDMode {
    uint8_t len;
    struct LightState states[MAX_MODES_COUNT];
};

struct LastState {
    uint8_t state[8];
    uint32_t elapsed_state_time[MAX_MODES_COUNT];
};

typedef void (* set_led_function)(bool);

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UART_TIMEOUT 10
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

const set_led_function led_functions[] = {
    [LED_NO_ONE] = set_no_one_led,
    [LED_RED] = set_red_led,
    [LED_GREEN] = set_green_led,
    [LED_YELLOW] = set_yellow_led,
};


struct LEDMode modes[8] = {
		{
			.len = 2,
            .states = {
                {
                    .color = LED_GREEN,
                    .timeout = MEDIUM,
                },
                {
                    .color = LED_YELLOW,
                    .timeout = MEDIUM,
                },
            },
        },
        {
            .len = 6,
            .states = {
                {
                    .color = LED_RED,
                    .timeout = MEDIUM,
                },
                {
                    .color = LED_NO_ONE,
                    .timeout = MEDIUM,
                },
                {
                    .color = LED_YELLOW,
                    .timeout = MEDIUM,
                },
                {
                    .color = LED_NO_ONE,
                    .timeout = MEDIUM,
                },
                {
                    .color = LED_GREEN,
                    .timeout = MEDIUM,
                },
                {
                    .color = LED_NO_ONE,
                    .timeout = MEDIUM,
                },
            },
        },
        {
            .len = 4,
            .states = {
                {
                    .color = LED_GREEN,
                    .timeout = SLOW,
                },
                {
                    .color = LED_NO_ONE,
                    .timeout = MEDIUM,
                },
                {
                    .color = LED_RED,
                    .timeout = SLOW,
                },
                {
                    .color = LED_NO_ONE,
                    .timeout = MEDIUM,
                },
            },
        },
        {
            .len = 6,
            .states = {
                {
                    .color = LED_RED,
                    .timeout = SLOW,
                },
                {
                    .color = LED_NO_ONE,
                    .timeout = SLOW,
                },
                {
                    .color = LED_YELLOW,
                    .timeout = SLOW,
                },
                {
                    .color = LED_NO_ONE,
                    .timeout = SLOW,
                },
                {
                    .color = LED_GREEN,
                    .timeout = SLOW,
                },
                {
                    .color = LED_NO_ONE,
                    .timeout = SLOW,
                },
            },
        }
    };

struct LEDMode new_mode;

struct LastState last_state = {
    .state = { 0 },
    .elapsed_state_time = { 0 },
};

void transmit_async(const char * content) {
	content_all = content;
	HAL_UART_Transmit_IT(&huart6, (uint8_t *) content, strlen(content));
}

void transmit(const char * content) {
	HAL_UART_Transmit(&huart6, (uint8_t *) content, strlen(content), UART_TIMEOUT);
}

void print(const char * content) {
	if (is_interrupt_mode) {
		transmit_async(content);
	} else {
		transmit(content);
	}
}

void println(const char * message) {
	print(message);
	print("\r\n");
}

void print_format(const char * format, ...) {
	static char buffer[1024];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);
	println(buffer);
}

void set_active_mode(uint8_t mode_number) {
	led_functions[modes[cur_mode_code].states[last_state.state[cur_mode_code]].color](false);

	cur_mode_code = mode_number;

	if (modes[cur_mode_code].len > 0) {
		led_functions[modes[cur_mode_code].states[last_state.state[cur_mode_code]].color](true);
	}
}

bool handle_set_command() {
	const char * const mode_idx_str = cmd + 4; // set pointer after 'new '

	uint32_t mode_idx;
	if ((sscanf(mode_idx_str, "%lu", &mode_idx) != 1) || ((mode_idx < 1 || mode_idx > current_max_mode + 1) && mode_idx < MAX_MODES_COUNT)) {
		return false;
	}

	set_active_mode(mode_idx - 1);
	return true;
}

bool handle_new_command() {
    char* pattern = cmd + 4; // set pointer after 'new '; example: rngyn
	uint32_t pattern_length = strlen(pattern);

	if (pattern_length < 2 || pattern_length > 8) {
		return false;
	}

	new_mode.len = pattern_length;

	for (uint8_t i = 0; i < pattern_length; ++i)
		switch (pattern[i]) {
			case 'n':
				new_mode.states[i].color = LED_NO_ONE;
				break;
			case 'r':
				new_mode.states[i].color = LED_RED;
				break;
			case 'g':
				new_mode.states[i].color = LED_GREEN;
				break;
			case 'y':
				new_mode.states[i].color = LED_YELLOW;
				break;
			default:
				return false;
		}

	remaining_timeouts_input = pattern_length;

	print_format("Creating new mode with length of %d. Please specify timeouts for each state (slow, medium, fast):\r\n", pattern_length);

	return true;
}

bool handle_new_command_timeout() {
	const uint8_t state_idx = new_mode.len - remaining_timeouts_input;

	if (strlen(cmd) == 0) { // no code after "new"
		return false;
	}

	if (string_equals("slow", cmd)) {
		new_mode.states[state_idx].timeout = SLOW;
	}

	else if (string_equals("medium", cmd)) {
		new_mode.states[state_idx].timeout = MEDIUM;
	}

	else if (string_equals("fast", cmd)) {
		new_mode.states[state_idx].timeout = FAST;
	}

	else {
		return false;
	}

	--remaining_timeouts_input;

	if (remaining_timeouts_input == 0) {
		adding_mode = CONST_MODES_COUNT + (adding_mode + 1 - CONST_MODES_COUNT) % (MAX_MODES_COUNT - CONST_MODES_COUNT + 1);

		const uint8_t mode_idx = adding_mode;

		memcpy(modes + mode_idx, &(new_mode), sizeof(new_mode));

		print_format("The mode %d is created\r\n", mode_idx + 1);

		current_max_mode = (current_max_mode < MAX_MODES_COUNT - 1) ? adding_mode : current_max_mode;

		return true;
	}

	print_format("Added mode: %d \r\n", adding_mode);

	print_format("%d timeouts are remaining:\r\n", remaining_timeouts_input);

	return true;
}

void handle_command_line() {
	bool success = false;

	if (strlen(cmd) == 0) {
		return; // empty line
	}

	if (string_equals("set interrupts on", cmd)) {
		is_interrupt_mode = true;
		success = true;
	}

	else if (string_equals("set interrupts off", cmd)) {
		is_interrupt_mode = false;
		HAL_UART_AbortReceive(&huart6);
		HAL_UART_Abort_IT(&huart6);
		success = true;
	}

	else if (starts_with("set ", cmd)) {
		success = handle_set_command();
	}

	else if (starts_with("new ", cmd)) {
		success = handle_new_command();
	}

	else if (remaining_timeouts_input > 0) {
		success = handle_new_command_timeout();
	}

	else {
		success = false;
	}

	println(success ? OK_MESSAGE : WRONG_COMMAND);
}

void receive_char_async() {
	HAL_UART_Receive_IT(
		&huart6,
		(uint8_t*) &last_received_char,
		sizeof(last_received_char)
	);
}

bool receive_char() {
	auto result = HAL_UART_Receive(
		&huart6,
		(uint8_t*) &last_received_char,
		sizeof(last_received_char),
		UART_TIMEOUT
	);

	return result == HAL_OK;
}

void delete_char_from_buffer() {
	const uint8_t cmd_len = strlen(cmd);
	if (cmd_len > 0) {
		cmd[cmd_len - 1] = '\0';
	}
}

void clear_buffer() {
	memset(cmd, '\0', sizeof(cmd));
}

void readln() {
    if (is_interrupt_mode) {
        while(data_available()) {
            receive_char_async();
            return;
        }
    } else {
    	if (!receive_char()) { // try to receive synchronously
    		return;
    	}
    }

    // handle received char
    print(&last_received_char);

    switch (last_received_char) {
    	// backspace
    	case '\b':
    	case 0x7F: {
    		delete_char_from_buffer();
    		return;
    	}
    	// enter
    	case '\r':
    	    println("\n");
    		handle_command_line();
    		clear_buffer();
    		return;
    }
    const uint32_t command_line_length = strlen(cmd);

    // check buffer overflow
    if (command_line_length == sizeof(cmd) - 1) {
    	print_format("\r\n %s", WRONG_COMMAND);
    	clear_buffer();
    	return;
    }

    cmd[command_line_length] = last_received_char;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	last_received_char = buf_read();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	buf_sendstring(content_all);
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
  ring_buffer_init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    uint32_t current_time = HAL_GetTick();
    while (1) {
        if (is_btn_pressed(&last_pressed_time)) {
        	set_active_mode((cur_mode_code + 1) % CONST_MODES_COUNT);
        }

        readln();

        last_state.elapsed_state_time[cur_mode_code] += HAL_GetTick() - current_time;

        current_time = HAL_GetTick();

        const struct LEDMode* current_mode = modes + cur_mode_code;

        if (current_mode->len == 0) {
            continue;
        }

        const struct LightState* current_state = current_mode->states + last_state.state[cur_mode_code];

        if (last_state.elapsed_state_time[cur_mode_code] >= current_state->timeout) {
            led_functions[current_state->color](false);

            last_state.elapsed_state_time[cur_mode_code] = 0;

            last_state.state[cur_mode_code] = (last_state.state[cur_mode_code] + 1) % current_mode->len;

            led_functions[current_mode->states[last_state.state[cur_mode_code]].color](true);
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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
  /* User can add his own implementation to report the HAL error return last_state */

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
