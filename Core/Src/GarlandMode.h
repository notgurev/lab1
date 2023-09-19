#include <stdbool.h>
#include "main.h"
#include "gpio.h"
#include "utils.h"

struct GarlandMode {
	int len; // length of codes array
	int codes[100]; // codes array, max 100 elements
	uint32_t delay_time; // delay time when code == WAIT
	int current_code_index; // index of current code
	GPIO_PinState led_state[3]; // green, yellow, red
};

void GarlandMode_run(struct GarlandMode* mode, uint32_t* last_pressed_time);
