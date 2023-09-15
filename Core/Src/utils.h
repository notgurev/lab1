void toggleLED(int code);

void setLED(int code, bool state);

void reset_LEDs();

bool is_btn_pressed(uint32_t* last_pressed_time);

void sleep(int ms);

#define WAIT -1
#define GREEN GPIO_PIN_13
#define YELLOW GPIO_PIN_14
#define RED GPIO_PIN_15
