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
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "utils.h"
#include "UartRingbuffer.h"
#include "mode.c"
#include <string.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MODES_LENGTH 9
#define TIMEOUT 10

#define KB_I2C_ADDRESS (0xE2)
#define KB_I2C_READ_ADDRESS ((KB_I2C_ADDRESS) | 1)
#define KB_I2C_WRITE_ADDRESS ((KB_I2C_ADDRESS) & ~1)
#define KB_INPUT_REG (0x0)
#define KB_OUTPUT_REG (0x1)
#define KB_CONFIG_REG (0x3)
#define KB_KEY_DEBOUNCE_TIME (100)
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

struct Mode buffer_mode = {LED_GREEN, 0};
int input_index = 0;

bool is_setting_mode = false;
uint32_t last_pressing_time = 0;
int last_peressed_btn_index = -1;

bool is_digit_input_mode = true;
bool is_test_keyboard_mode = false;
bool last_btn_state = false;

char digits[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 'q', '0', '\r'};
char chars[] = {'a', 'b', 'c', '+', '-', '!', '!', '!', '!', 'q', '!', '\r'};

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
		case 0 :
			htim4.Instance->CCR2 = 10 * brightness;
			break;
		case 1 :
			htim4.Instance->CCR3 = 10 * brightness;
			break;
		case 2 :
			htim4.Instance->CCR4 = 10 * brightness;
			break;
	}
}

void print(const char * content) {
	uart_sendstring(content);
}

void print_number(const int content) {
	char str_index[3];
	sprintf(str_index, "%i", content);
	print(str_index);
}

void print_mode_description(struct Mode mode, int index, bool is_editing_mode){
	if (is_editing_mode) print("Editing mode ");
	else print("Mode ");
	print_number(index + 1);
	print(": ");
	switch (mode.led) {
		case 0 : print("green, "); break;
		case 1 : print("yellow, "); break;
		case 2 : print("red, "); break;
	}
	print_number(mode.brightness);
	print("% brightness\n\r");
}

int get_peressed_btn_index(){
	const uint32_t t = HAL_GetTick();
	if (t - last_pressing_time < KB_KEY_DEBOUNCE_TIME) return -1;
	int index = -1;
	uint8_t reg_buffer = ~0;

	uint8_t tmp = 0;
	HAL_I2C_Mem_Write(&hi2c1, KB_I2C_WRITE_ADDRESS, KB_OUTPUT_REG, 1, &tmp, 1, KB_KEY_DEBOUNCE_TIME);

	for (int row = 0; row < 4; row++) {
		uint8_t buf = ~((uint8_t) (1 << row));

		HAL_I2C_Mem_Write(&hi2c1, KB_I2C_WRITE_ADDRESS, KB_CONFIG_REG, 1, &buf, 1, KB_KEY_DEBOUNCE_TIME);

		HAL_Delay(10);

		flag = HAL_I2C_Mem_Read(&hi2c1, KB_I2C_READ_ADDRESS, KB_INPUT_REG, 1, &reg_buffer, 1, KB_KEY_DEBOUNCE_TIME);

		switch(reg_buffer >> 4){
			case 6: index = row * 3 + 1; break;
			case 5: index = row * 3 + 2; break;
			case 3: index = row * 3 + 3; break;
		}
	}
	if (index != -1) last_pressing_time = t;

	if (index == last_peressed_btn_index){
		return -1;
	}

	last_peressed_btn_index = index;
	return index;
}

char key2char(const int key){
    return is_digit_input_mode ? digits[key - 1] : chars[key - 1];
}

void print_char_value(const char * array, int i){
	if (array[i] == 'q'){
		print("change keyboard layout");
	}else if (array[i] == '\r'){
		print("enter");
	}else if (array[i] != '!'){
		char s[2];
		s[0] = array[i];
		s[1] = '\0';
		print(s);
	}
}

void print_key_value(int i){
	print("Digit mode: ");
	print_char_value(digits, i);
	print(", Char mode: ");
	print_char_value(chars, i);
	print("\r\n");
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
  MX_I2C1_Init();
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
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  bool btn_state = is_btn_press();
	  if (last_btn_state && !btn_state){
		  is_test_keyboard_mode = !is_test_keyboard_mode;
		  if (is_test_keyboard_mode) print("Test keyboard mode is on\n\r");
		  else print("Test keyboard mode is off\n\r");
	  }
	  last_btn_state = btn_state;

	  int btn_index = get_peressed_btn_index();
	  if (btn_index != -1) {
		char received_char = key2char(btn_index);
		if (is_test_keyboard_mode) {
			print_key_value(btn_index - 1);
		} else if (is_setting_mode) {
			if (received_char == '\r') {
				is_setting_mode = false;
				MODES[input_index].led = buffer_mode.led;
				MODES[input_index].brightness = buffer_mode.brightness;
				print("Mode is saved\n\r");
				print("Setting mode is off\n\r");
				if (!is_digit_input_mode) {
					is_digit_input_mode = true;
					print("Digit input mode is on\n\r");
				}
			} else {
				bool changed = true;
				int i = atoi(&received_char) - 1;
				if (i >= 0) {
					input_index = i;
					buffer_mode.led = MODES[input_index].led;
					buffer_mode.brightness = MODES[input_index].brightness;
				} else switch (received_char) {
					case 'a' :
						buffer_mode.led = LED_GREEN;
						break;
					case 'b' :
						buffer_mode.led = LED_YELLOW;
						break;
					case 'c' :
						buffer_mode.led = LED_RED;
						break;
					case 'q' :
						changed = false;
						is_digit_input_mode = !is_digit_input_mode;
						if (is_digit_input_mode) print("Digit input mode is on\n\r");
						else print("Char input mode is on\n\r");
						break;
					case '-' :
						buffer_mode.brightness = buffer_mode.brightness >= 10 ? buffer_mode.brightness - 10 : 0;
						break;
					case '+' :
						buffer_mode.brightness = buffer_mode.brightness <= 90 ? buffer_mode.brightness + 10 : 100;
						break;
					default:
						changed = false;
						break;
				}
				if (changed) print_mode_description(buffer_mode, input_index, true);
			}
		} else {
			if (received_char == '\r') {
				is_setting_mode = true;
				print("Setting mode is on\n\r");
			} else {
				int mode_index = atoi(&received_char) - 1;
				if (received_char != 'q') {
					if (mode_index != -1) {
						set_pin(MODES[mode_index].led, MODES[mode_index].brightness);
						print_mode_description(MODES[mode_index], mode_index, false);
					} else {
						set_pin(-1, 0);
						print("Every pin is off\n\r");
					}
				}
			}
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
