
#include "SPI.h"
#include <miosix.h>

using namespace std;
using namespace miosix;

#ifdef _BOARD_STM32VLDISCOVERY

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

unsigned char SPI::sendRecv(unsigned char data)
{
    SPI1->DR = data;
    while ((SPI1->SR & SPI_SR_RXNE) == 0); //Wait
    return SPI1->DR;
}

#elif defined(_BOARD_POLINODE)

SPI::SPI()
{
    FastInterruptDisableLock dLock;
    CMU->HFPERCLKEN0|=CMU_HFPERCLKEN0_USART1;
    USART1->CTRL=USART_CTRL_MSBF
               | USART_CTRL_SYNC;
    USART1->FRAME=USART_FRAME_STOPBITS_ONE //Should not even be needed
                | USART_FRAME_PARITY_NONE
                | USART_FRAME_DATABITS_EIGHT;
    USART1->CLKDIV=((48000000/8000000)-1)<<8; //CC2520 max freq is 8MHz
    USART1->IEN=0;
    USART1->IRCTRL=0;
    USART1->I2SCTRL=0;
    USART1->ROUTE=USART_ROUTE_LOCATION_LOC1
                | USART_ROUTE_CLKPEN
                | USART_ROUTE_TXPEN
                | USART_ROUTE_RXPEN;
    USART1->CMD=USART_CMD_CLEARRX
              | USART_CMD_CLEARTX
              | USART_CMD_TXTRIDIS
              | USART_CMD_RXBLOCKDIS
              | USART_CMD_MASTEREN
              | USART_CMD_TXEN
              | USART_CMD_RXEN;
}

unsigned char SPI::sendRecv(unsigned char data)
{
    USART1->TXDATA=data;
    while((USART1->STATUS & USART_STATUS_RXDATAV)==0) ;
    return USART1->RXDATA;
}

void SPI::enable()
{
    USART1->ROUTE=USART_ROUTE_LOCATION_LOC1
                | USART_ROUTE_CLKPEN
                | USART_ROUTE_TXPEN
                | USART_ROUTE_RXPEN;
    USART1->CMD=USART_CMD_CLEARRX
              | USART_CMD_CLEARTX
              | USART_CMD_TXTRIDIS
              | USART_CMD_RXBLOCKDIS
              | USART_CMD_MASTEREN
              | USART_CMD_TXEN
              | USART_CMD_RXEN;
}

void SPI::disable()
{
    USART1->CMD=USART_CMD_TXDIS | USART_CMD_RXDIS;
    USART1->ROUTE=0;
    internalSpi::mosi::mode(Mode::OUTPUT_LOW);
    internalSpi::miso::mode(Mode::INPUT_PULL_DOWN); //To prevent it from floating
    internalSpi::sck::mode(Mode::OUTPUT_LOW);
}

#endif
