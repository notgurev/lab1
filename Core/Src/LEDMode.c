typedef struct LEDMode {
	int len;
	int code[100];
	int delay_time;
	int current_code_index;
	int ledState[3]; // green, yellow, red
};
