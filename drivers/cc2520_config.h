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
    typedef miosix::Gpio<GPIOC_BASE, 4> fifo_irq; //GPIO1
    typedef miosix::Gpio<GPIOB_BASE, 2> fifop_irq; //GPIO2
    typedef miosix::Gpio<GPIOB_BASE, 0> exc_channelB; //GPIO3
    typedef miosix::Gpio<GPIOC_BASE, 5> sfd; //GPIO4
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
        cc2520::fifo_irq::mode(miosix::Mode::INPUT);
        cc2520::fifop_irq::mode(miosix::Mode::INPUT);
        cc2520::sfd::mode(miosix::Mode::INPUT);
        cc2520::exc_channelB::mode(miosix::Mode::INPUT);
        cc2520::sck::mode(miosix::Mode::ALTERNATE);
        cc2520::miso::mode(miosix::Mode::ALTERNATE);
        cc2520::mosi::mode(miosix::Mode::ALTERNATE);
        cc2520::vreg::mode(miosix::Mode::OUTPUT);
        cc2520::vreg::low();
        cc2520::reset::mode(miosix::Mode::OUTPUT);
        cc2520::reset::high();
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;         //enable AFIO reg
        AFIO->EXTICR[1] |= AFIO_EXTICR2_EXTI6_PA;   //enable interrupt on PA6
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
        NVIC_SetPriority(EXTI9_5_IRQn,10); //low priority
        NVIC_EnableIRQ(EXTI9_5_IRQn);
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

#elif defined(_BOARD_WANDSTEM)
/*
 * Connections between CC2520 and stm32vldiscovery
 */
namespace cc2520 {
    typedef miosix::transceiver::cs     cs;
    typedef miosix::transceiver::gpio1  fifo_irq; //GPIO1
    typedef miosix::transceiver::gpio2  fifop_irq; //GPIO2
    typedef miosix::transceiver::excChB exc_channelB; //GPIO3
    #if WANDSTEM_HW_REV<13
    typedef miosix::transceiver::gpio4  sfd; //GPIO4
    #endif
    typedef miosix::internalSpi::sck    sck;
    typedef miosix::internalSpi::miso   miso;
    typedef miosix::internalSpi::mosi   mosi;
    typedef miosix::transceiver::vregEn vreg;
    typedef miosix::transceiver::reset  reset; 
}

/**
 * Setup GPIOs that connect the CC2520 to the microcontroller
 */
static inline void cc2520GpioInit() {
    {
        miosix::FastInterruptDisableLock dLock;
        //mosi, miso, sck, cs already configured by bsp
        cc2520::fifo_irq::mode(miosix::Mode::INPUT);
        cc2520::fifop_irq::mode(miosix::Mode::INPUT);
        #if WANDSTEM_HW_REV<13
        cc2520::sfd::mode(miosix::Mode::INPUT);
        #endif
        
        cc2520::exc_channelB::mode(miosix::Mode::INPUT);
        cc2520::vreg::mode(miosix::Mode::OUTPUT_LOW);
        cc2520::reset::mode(miosix::Mode::OUTPUT_HIGH);
        
        CMU->HFPERCLKEN0|=CMU_HFPERCLKEN0_USART1;
        
        NVIC_EnableIRQ(GPIO_ODD_IRQn);
        NVIC_SetPriority(GPIO_ODD_IRQn,15); //Low priority
    }
    //Configuration SPI
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

/**
 * Send/receive one byte from the SPI interface where the CC2520 is attached
 * \param data byte to send
 * \return byte received
 */
static unsigned char cc2520SpiSendRecv(unsigned char data = 0) {
    USART1->TXDATA=data;
    while((USART1->STATUS & USART_STATUS_RXDATAV)==0) ;
    return USART1->RXDATA;
}

#endif

#endif //CC2520_CONFIG_H
