
#ifndef LEDS_H
#define	LEDS_H

#include <interfaces/bsp.h>

#ifdef _BOARD_STM32VLDISCOVERY
typedef miosix::Gpio<GPIOC_BASE,9> led1; //Green LED
typedef miosix::Gpio<GPIOC_BASE,8> led2; //Blue LED
#elif defined(_BOARD_POLINODE)
typedef miosix::greenLed led1;
typedef miosix::redLed   led2;
#endif

#endif //LEDS_H
