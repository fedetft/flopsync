
/*
 * FIXME: this file is a modified copy-paste of cc2520_config.h
 * used by
 * test_barraled_TX.cpp
 * test_barraled_RX.cpp
 * 
 * Modify those files to use the original cc2520_config.h and delete this
 * 
 */

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
//    typedef miosix::Gpio<GPIOB_BASE, 0> exc_channelB; //GPIO3
//    typedef miosix::Gpio<GPIOC_BASE, 5> sfd; //GPIO4
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
//        cc2520::cs::mode(miosix::Mode::OUTPUT);
//        cc2520::cs::high();
        cc2520::fifo_irq::mode(miosix::Mode::INPUT);
        cc2520::fifop_irq::mode(miosix::Mode::INPUT);
//        cc2520::sfd::mode(miosix::Mode::INPUT);
//        cc2520::exc_channelB::mode(miosix::Mode::INPUT);
//        cc2520::sck::mode(miosix::Mode::ALTERNATE);
//        cc2520::miso::mode(miosix::Mode::ALTERNATE);
//        cc2520::mosi::mode(miosix::Mode::ALTERNATE);
//        cc2520::vreg::mode(miosix::Mode::OUTPUT);
//        cc2520::vreg::low();
//        cc2520::reset::mode(miosix::Mode::OUTPUT);
//        cc2520::reset::high();
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;         //enable AFIO reg
        AFIO->EXTICR[1] |= AFIO_EXTICR2_EXTI6_PA;   //enable interrupt on PA6
//        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
        NVIC_SetPriority(EXTI9_5_IRQn,10); //low priority
        NVIC_EnableIRQ(EXTI9_5_IRQn);
    }
//    //Configuration SPI
//    SPI1->CR1 = SPI_CR1_SSM //CS handled in software
//            | SPI_CR1_SSI //Internal CS high
//            | SPI_CR1_BR_0 //24/4=6MHz (<(8MHz max cc2520 speed)
//            | SPI_CR1_SPE //SPI enabled
//            | SPI_CR1_MSTR; //Master mode
}

///**
// * Send/receive one byte from the SPI interface where the CC2520 is attached
// * \param data byte to send
// * \return byte received
// */
//static unsigned char cc2520SpiSendRecv(unsigned char data = 0) {
//    SPI1->DR = data;
//    while ((SPI1->SR & SPI_SR_RXNE) == 0); //Wait
//    return SPI1->DR;
//}

#elif defined(_BOARD_STM3220G_EVAL)

//TBD

#endif

#endif //CC2520_CONFIG_H
