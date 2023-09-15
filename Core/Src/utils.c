#include <stdbool.h>
#include "gpio.h"

void toggleLED(int code) {
	HAL_GPIO_TogglePin(GPIOD, code);
}

void setLED(int code, bool state) {
	HAL_GPIO_WritePin(GPIOD, code, state);
}

bool is_btn_pressed(uint32_t* time) {
	int last_press_time = *time;

	 if (!HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_15)) { // is pressed
		 *time = HAL_GetTick();
		 if (*time - last_press_time > DELAY) {
			 return true;
		 }
	 }

	 return false;
}

void reset_LEDs() {
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 0);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 0);
}
