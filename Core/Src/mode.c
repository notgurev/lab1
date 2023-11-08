#define LENGTH 8

typedef struct Mode {
	int code[LENGTH];
	int size;
	int delay;
	int current_code_index;
};


enum LED {
    LED_GREEN = 0,
    LED_YELLOW = 1,
	LED_RED = 2,
	LED_NO_ONE
};
