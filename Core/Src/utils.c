#include <stdbool.h>
#include "gpio.h"

void toggleLED(int code) {
	HAL_GPIO_TogglePin(GPIOD, code);
}

// State is GPIO_PIN_SET or GPIO_PIN_RESET
void setLED(int code, bool state) {
	HAL_GPIO_WritePin(GPIOD, code, state);
}

// Resets all three LEDs
void reset_LEDs() {
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);
}
