
#ifndef LEDS_H
#define	LEDS_H

#include <interfaces/bsp.h>

#ifdef _BOARD_ALS_MAINBOARD
typedef miosix::Gpio<GPIOC_BASE,9> led1; //Green LED
typedef miosix::Gpio<GPIOC_BASE,8> led2; //Blue LED
#elif defined(_BOARD_POLINODE)
typedef miosix::redLed   led1;
typedef miosix::greenLed led2;
#endif

#endif //LEDS_H
