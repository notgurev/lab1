#include "main.h"
#include "gpio.h"
#include "LEDMode.h"
#include "utils.h"

int LEDMode_activate(struct LEDMode* mode, uint32_t* last_pressed_time) {
	// Get current code, such as GREEN or WAIT
	uint32_t led = mode->code[mode->current_code_index];

	// Set to next mode
	mode->current_code_index++;

    if (mode->current_code_index > mode->len) {
	    mode->current_code_index = 0;
	}

	if (led == WAIT) {
		int start_time = HAL_GetTick();

		// Wait for delay_time, while checking if button is pressed
		while (HAL_GetTick() < start_time + mode->delay_time) {
			if (is_btn_pressed(last_pressed_time)) {
				return 1;
			}
		}
	} else {
		switch (led) {
		case GREEN:
			mode->ledState[0] = !mode->ledState[0];
			break;
		case YELLOW:
			mode->ledState[1] = !mode->ledState[1];
			break;
		case RED:
			mode->ledState[2] = !mode->ledState[2];
			break;
		}

		toggleLED(led);
	}

	return 0;
};
