
#ifndef FAST_TRIGGER_H
#define FAST_TRIGGER_H

#ifndef _BOARD_STM32VLDISCOVERY
#error "This header can only be included when compiling for stm32vldiscovery"
#endif

extern "C" {

void trigger_80ns();
void trigger_160ns();
void trigger_320ns();
void trigger_640ns();

}

#endif


