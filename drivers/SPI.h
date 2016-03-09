#ifndef SPI_H
#define SPI_H

#include "HW_mapping.h"

class SPI
{
public:

    SPI();

    unsigned char sendRecv(unsigned char data = 0);
    
    #ifdef _BOARD_WANDSTEM
    void enable();
    void disable();
    #endif

private:

    #ifdef _BOARD_STM32VLDISCOVERY
    typedef miosix::Gpio<GPIOA_BASE, 6> miso;
    typedef miosix::Gpio<GPIOA_BASE, 7> mosi;
    #endif
};

#endif // SPI_H
