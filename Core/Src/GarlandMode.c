#include "GarlandMode.h"

#include "main.h"
#include "gpio.h"
#include "utils.h"

// Runs the garland with provided mode. Returns when button is pressed.
void GarlandMode_run(struct GarlandMode* mode, uint32_t* last_pressed_time) {
	while (1) {
		// Get current code, such as GREEN or WAIT
		uint32_t led = mode->codes[mode->current_code_index];

		if (led == WAIT) {
			uint32_t start_time = HAL_GetTick();

			// Wait for delay_time, while checking if button is pressed
			while (HAL_GetTick() < start_time + mode->delay_time) {
				if (is_btn_pressed(last_pressed_time)) {
					return;
				}
			}
		} else {
			switch (led) {
			case GREEN:
				mode->led_state[0] = !mode->led_state[0];
				break;
			case YELLOW:
				mode->led_state[1] = !mode->led_state[1];
				break;
			case RED:
				mode->led_state[2] = !mode->led_state[2];
				break;
			}

			toggle_LED(led);
		}

		// Set to next code
		mode->current_code_index++;
		if (mode->current_code_index > mode->len) {
		    mode->current_code_index = 0;
		}
	}
};
