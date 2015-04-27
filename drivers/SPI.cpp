#include "SPI.h"

SPI::SPI()
{

        cc2520::sck::mode(miosix::Mode::ALTERNATE);
        cc2520::miso::mode(miosix::Mode::ALTERNATE);
        cc2520::mosi::mode(miosix::Mode::ALTERNATE);

        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        RCC_SYNC();

        SPI1->CR1 = SPI_CR1_SSM //CS handled in software
            | SPI_CR1_SSI //Internal CS high
            | SPI_CR1_BR_0 //24/4=6MHz (<(8MHz max cc2520 speed)
            | SPI_CR1_SPE //SPI enabled
            | SPI_CR1_MSTR; //Master mode
}

unsigned char SPI::sendRecv(unsigned char data) {

SPI1->DR = data;
while ((SPI1->SR & SPI_SR_RXNE) == 0); //Wait
return SPI1->DR;
}
