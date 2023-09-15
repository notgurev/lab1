#include <stdbool.h>
#include "main.h"
#include "gpio.h"
#include "utils.h"

struct GarlandMode {
	int len; // length of codes array
	int code[100]; // codes array, max 100 elements
	int delay_time; // delay time when code == WAIT
	int current_code_index; // index of current code
	int ledState[3]; // green, yellow, red
};

int GarlandMode_run(struct GarlandMode* mode, uint32_t* last_pressed_time);
