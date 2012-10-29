/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef NRF24L01_CONFIG_H
#define	NRF24L01_CONFIG_H

#include <miosix.h>

#ifdef _BOARD_ALS_MAINBOARD

/**
 * Setup GPIOs that connect the NRF24L01+ to the microcontroller
 */
static inline void nrfGpioInit()
{
    //Nothing to do, gpio setup already taken care of by the bsp
}

/**
 * Send/receive one byte from the SPI interface where the NRF24L01+ is attached
 * \param data byte to send
 * \return byte received
 */
static unsigned char nrfSpiSendRecv(unsigned char data=0)
{
    SPI1->DR=data;
    while((SPI1->SR & SPI_SR_RXNE)==0) ; //Wait
    return SPI1->DR;
}

#elif defined(_BOARD_STM3210E_EVAL)

/*
 * Connections between nRF24L01+ and stm3210e-eval
 */
namespace nrf {
typedef miosix::Gpio<GPIOC_BASE,1> ce;
typedef miosix::Gpio<GPIOC_BASE,2> cs;
typedef miosix::Gpio<GPIOC_BASE,3> irq;
typedef miosix::Gpio<GPIOA_BASE,5> sck;
typedef miosix::Gpio<GPIOA_BASE,6> miso;
typedef miosix::Gpio<GPIOA_BASE,7> mosi;
}

/**
 * Setup GPIOs that connect the NRF24L01+ to the microcontroller
 */
static inline void nrfGpioInit()
{
    {
        miosix::FastInterruptDisableLock dLock;
        nrf::ce::mode(miosix::Mode::OUTPUT);
        nrf::ce::low();
        nrf::cs::mode(miosix::Mode::OUTPUT);
        nrf::cs::high();
        nrf::irq::mode(miosix::Mode::INPUT);
        nrf::sck::mode(miosix::Mode::ALTERNATE);
        nrf::miso::mode(miosix::Mode::ALTERNATE);
        nrf::mosi::mode(miosix::Mode::ALTERNATE);
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    }
    SPI1->CR1=SPI_CR1_SSM  //CS handled in software
            | SPI_CR1_SSI  //Internal CS high
            | SPI_CR1_BR_1 //72/8=9MHz (<10MHz max nRF speed)
            | SPI_CR1_SPE  //SPI enabled
            | SPI_CR1_MSTR;//Master mode
}

/**
 * Send/receive one byte from the SPI interface where the NRF24L01+ is attached
 * \param data byte to send
 * \return byte received
 */
static unsigned char nrfSpiSendRecv(unsigned char data=0)
{
    SPI1->DR=data;
    while((SPI1->SR & SPI_SR_RXNE)==0) ; //Wait
    return SPI1->DR;
}

#elif defined(_BOARD_STM32VLDISCOVERY)

/*
 * Connections between nRF24L01+ and stm3210e-eval
 */
namespace nrf {
typedef miosix::Gpio<GPIOA_BASE,1> ce;
typedef miosix::Gpio<GPIOA_BASE,2> cs;
typedef miosix::Gpio<GPIOB_BASE,0> irq;
typedef miosix::Gpio<GPIOA_BASE,5> sck;
typedef miosix::Gpio<GPIOA_BASE,6> miso;
typedef miosix::Gpio<GPIOA_BASE,7> mosi;
}

/**
 * Setup GPIOs that connect the NRF24L01+ to the microcontroller
 */
static inline void nrfGpioInit()
{
    {
        miosix::FastInterruptDisableLock dLock;
        nrf::ce::mode(miosix::Mode::OUTPUT);
        nrf::ce::low();
        nrf::cs::mode(miosix::Mode::OUTPUT);
        nrf::cs::high();
        nrf::irq::mode(miosix::Mode::INPUT);
        nrf::sck::mode(miosix::Mode::ALTERNATE);
        nrf::miso::mode(miosix::Mode::ALTERNATE);
        nrf::mosi::mode(miosix::Mode::ALTERNATE);
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    }
    SPI1->CR1=SPI_CR1_SSM  //CS handled in software
            | SPI_CR1_SSI  //Internal CS high
            | SPI_CR1_BR_0 //24/4=6MHz (<10MHz max nRF speed)
            | SPI_CR1_SPE  //SPI enabled
            | SPI_CR1_MSTR;//Master mode
}

/**
 * Send/receive one byte from the SPI interface where the NRF24L01+ is attached
 * \param data byte to send
 * \return byte received
 */
static unsigned char nrfSpiSendRecv(unsigned char data=0)
{
    SPI1->DR=data;
    while((SPI1->SR & SPI_SR_RXNE)==0) ; //Wait
    return SPI1->DR;
}

#elif defined(_BOARD_STM3220G_EVAL)

//TBD

#endif

#endif //NRF24L01_CONFIG_H
