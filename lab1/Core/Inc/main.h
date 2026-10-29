#ifndef MAIN_H
#define MAIN_H

#include "stm32f4xx_hal.h"

#define LED1_GPIO_Port GPIOB
#define LED1_Pin       GPIO_PIN_1

void Error_Handler(void);

#endif /* MAIN_H */
