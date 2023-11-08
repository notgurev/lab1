#include <stdbool.h>
#include "gpio.h"

bool is_btn_press() {
	 return HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_15) == 0;
 }
