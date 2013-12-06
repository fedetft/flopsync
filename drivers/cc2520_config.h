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

#ifndef CC2520_CONFIG_H
#define	CC2520_CONFIG_H


#include <miosix.h>

#ifdef _BOARD_ALS_MAINBOARD

//TBD

#elif defined(_BOARD_STM3210E_EVAL)

//TBD

#elif defined(_BOARD_STM32VLDISCOVERY)

/*
 * Connections between CC2520 and stm32vldiscovery
 */
namespace cc2520 {
    typedef miosix::Gpio<GPIOA_BASE, 2> cs;
    typedef miosix::Gpio<GPIOB_BASE, 0> rx_irq; //GPIO1
    typedef miosix::Gpio<GPIOB_BASE, 1> sfd; //GPIO4
    typedef miosix::Gpio<GPIOA_BASE, 5> sck;
    typedef miosix::Gpio<GPIOA_BASE, 6> miso;
    typedef miosix::Gpio<GPIOA_BASE, 7> mosi;
    typedef miosix::Gpio<GPIOA_BASE, 3> vreg;
    typedef miosix::Gpio<GPIOA_BASE, 1> reset; 
}

/**
 * Setup GPIOs that connect the CC2520 to the microcontroller
 */
static inline void cc2520GpioInit() {
    {
        miosix::FastInterruptDisableLock dLock;
        cc2520::cs::mode(miosix::Mode::OUTPUT);
        cc2520::cs::high();
        cc2520::rx_irq::mode(miosix::Mode::INPUT);
        cc2520::sfd::mode(miosix::Mode::INPUT);
        cc2520::sck::mode(miosix::Mode::ALTERNATE);
        cc2520::miso::mode(miosix::Mode::ALTERNATE);
        cc2520::mosi::mode(miosix::Mode::ALTERNATE);
        cc2520::vreg::mode(miosix::Mode::OUTPUT);
        cc2520::vreg::low();
        cc2520::reset::mode(miosix::Mode::OUTPUT);
        cc2520::reset::high();
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    }
    //Configuration SPI
    SPI1->CR1 = SPI_CR1_SSM //CS handled in software
            | SPI_CR1_SSI //Internal CS high
            | SPI_CR1_BR_0 //24/4=6MHz (<(8MHz max cc2520 speed)
            | SPI_CR1_SPE //SPI enabled
            | SPI_CR1_MSTR; //Master mode
}

/**
 * Send/receive one byte from the SPI interface where the CC2520 is attached
 * \param data byte to send
 * \return byte received
 */
static unsigned char cc2520SpiSendRecv(unsigned char data = 0) {
    SPI1->DR = data;
    while ((SPI1->SR & SPI_SR_RXNE) == 0); //Wait
    return SPI1->DR;
}

#elif defined(_BOARD_STM3220G_EVAL)

//TBD

#endif

#endif //CC2520_CONFIG_H