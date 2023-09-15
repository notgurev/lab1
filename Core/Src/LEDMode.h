struct LEDMode {
	int len; // length of codes array
	int code[100]; // codes array, max 100 elements
	int delay_time; // delay time when code == WAIT
	int current_code_index; // index of current code
	int ledState[3]; // green, yellow, red
};

int LEDMode_activate(struct LEDMode* mode, uint32_t* last_pressed_time);
