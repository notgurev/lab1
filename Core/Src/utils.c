#include <stdbool.h>
#include "gpio.h"

void toggleLED(int code) {
	HAL_GPIO_TogglePin(GPIOD, code);
}

// State is GPIO_PIN_SET or GPIO_PIN_RESET
void setLED(int code, bool state) {
	HAL_GPIO_WritePin(GPIOD, code, state);
}

// Sleeps for ms milliseconds
void sleep(int ms) {
	HAL_Delay(ms);
}

// Checks if button is pressed.
// Also checks value of HAL_GetTick to avoid rattle.
// Sets last_pressed_time if button is considered pressed.
bool is_btn_pressed(uint32_t* last_pressed_time) {
	// GPIO_PIN_RESET means pressed
	int pressed = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15) == GPIO_PIN_RESET;

	if (!pressed) {
		return false;
	}

	// If set too low, might trigger twice per click
	// If too high, might not register the second click
	const int RATTLE_TIME_MS = 500;

	int time_passed = HAL_GetTick() - *last_pressed_time;
	int rattle = time_passed < RATTLE_TIME_MS;

	if (rattle) {
		return false;
	}

	// Register press
	*last_pressed_time = HAL_GetTick();

	return true;
}

// Resets all three LEDs
void reset_LEDs() {
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);
}
