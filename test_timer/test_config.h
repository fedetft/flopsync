/* 
 * File:   test_config.h
 * Author: gigi
 *
 * Created on December 5, 2013, 7:46 PM
 */

#ifndef TEST_CONFIG_H
#define	TEST_CONFIG_H
#include <miosix.h>

inline void lowPowerSetup()
{
    using namespace miosix;
    //Disable JTAG
    //AFIO->MAPR=AFIO_MAPR_SWJ_CFG_2;
    
    //Put all unused GPIOs to a known low power state   
    //Gpio<GPIOA_BASE,0>::mode(Mode::INPUT_PULL_UP_DOWN); //user button
    //Gpio<GPIOA_BASE,1>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 reset
    //Gpio<GPIOA_BASE,2>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 cs
    //Gpio<GPIOA_BASE,3>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 vreg_en 
    Gpio<GPIOA_BASE,4>::mode(Mode::INPUT_PULL_UP_DOWN);
    //Gpio<GPIOA_BASE,5>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 sck
    //Gpio<GPIOA_BASE,6>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 miso requires 100k pulldown
    //Gpio<GPIOA_BASE,7>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 mosi
    Gpio<GPIOA_BASE,8>::mode(Mode::INPUT_PULL_UP_DOWN);
    //Gpio<GPIOA_BASE,9>::mode(Mode::INPUT_PULL_UP_DOWN); //Serial port
    //Gpio<GPIOA_BASE,10>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOA_BASE,11>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOA_BASE,12>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOA_BASE,13>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOA_BASE,14>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOA_BASE,15>::mode(Mode::INPUT_PULL_UP_DOWN);
    
    //Gpio<GPIOB_BASE,0>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 GPIO4
    //Gpio<GPIOB_BASE,1>::mode(Mode::INPUT_PULL_UP_DOWN); //RTC clock in
    //Gpio<GPIOB_BASE,2>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 fifop_irq
    Gpio<GPIOB_BASE,3>::mode(Mode::INPUT_PULL_UP_DOWN);
    //Gpio<GPIOB_BASE,4>::mode(Mode::INPUT_PULL_UP_DOWN); //trigger send packet
    Gpio<GPIOB_BASE,5>::mode(Mode::INPUT_PULL_UP_DOWN); 
    Gpio<GPIOB_BASE,6>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,7>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,8>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,9>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,10>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,11>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,12>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,13>::mode(Mode::INPUT_PULL_UP_DOWN); //debugger
    Gpio<GPIOB_BASE,14>::mode(Mode::INPUT_PULL_UP_DOWN); //debbuger
    Gpio<GPIOB_BASE,15>::mode(Mode::INPUT_PULL_UP_DOWN);
    
    Gpio<GPIOC_BASE,0>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOC_BASE,1>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOC_BASE,2>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOC_BASE,3>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOC_BASE,4>::mode(Mode::INPUT_PULL_UP_DOWN);
    //Gpio<GPIOC_BASE,5>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 sfd 
    Gpio<GPIOC_BASE,6>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOC_BASE,7>::mode(Mode::INPUT_PULL_UP_DOWN);
    //Gpio<GPIOC_BASE,8>::mode(Mode::INPUT_PULL_UP_DOWN); //Led
    //Gpio<GPIOC_BASE,9>::mode(Mode::INPUT_PULL_UP_DOWN); //Led
    Gpio<GPIOC_BASE,10>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOC_BASE,11>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOC_BASE,12>::mode(Mode::INPUT_PULL_UP_DOWN);
    //Gpio<GPIOC_BASE,13>::mode(Mode::INPUT_PULL_UP_DOWN); //RTC clock out
    //Gpio<GPIOC_BASE,14>::mode(Mode::INPUT_PULL_UP_DOWN); //32KHz XTAL
    //Gpio<GPIOC_BASE,15>::mode(Mode::INPUT_PULL_UP_DOWN);
}

#endif	/* TEST_CONFIG_H */

