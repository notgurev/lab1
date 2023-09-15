#include <stdbool.h>
#include "gpio.h"

void toggleLED(int code) {
	HAL_GPIO_TogglePin(GPIOD, code);
}

void setLED(int code, bool state) {
	HAL_GPIO_WritePin(GPIOD, code, state);
}

void reset_LEDs() {
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 0);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 0);
}
