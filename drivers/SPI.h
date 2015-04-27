#ifndef SPI_H
#define SPI_H

#include "HW_mapping.h"

class SPI
{
    public:

        SPI();

        unsigned char sendRecv(unsigned char data = 0);

    protected:
    private:

        typedef miosix::Gpio<GPIOA_BASE, 6> miso;
        typedef miosix::Gpio<GPIOA_BASE, 7> mosi;
};

#endif // SPI_H
