
#ifndef LOW_POWER_SETUP_H
#define LOW_POWER_SETUP_H

#include <miosix.h>

//inline because it's called once
inline void lowPowerSetup()
{
    using namespace miosix;
    //Disable JTAG
    AFIO->MAPR=AFIO_MAPR_SWJ_CFG_2;
    
    //Put all unused GPIOs to a known low power state   
    Gpio<GPIOA_BASE,0>::mode(Mode::INPUT_PULL_UP_DOWN);
    //Gpio<GPIOA_BASE,1>::mode(Mode::INPUT_PULL_UP_DOWN); //Nrf ce
    //Gpio<GPIOA_BASE,2>::mode(Mode::INPUT_PULL_UP_DOWN); //Nrf cs
    Gpio<GPIOA_BASE,3>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOA_BASE,4>::mode(Mode::INPUT_PULL_UP_DOWN);
    //Gpio<GPIOA_BASE,5>::mode(Mode::INPUT_PULL_UP_DOWN); //Nrf sck
    //Gpio<GPIOA_BASE,6>::mode(Mode::INPUT_PULL_UP_DOWN); //Nrf miso requires 100k pulldown
    //Gpio<GPIOA_BASE,7>::mode(Mode::INPUT_PULL_UP_DOWN); //Nrf mosi
    Gpio<GPIOA_BASE,8>::mode(Mode::INPUT_PULL_UP_DOWN);
    //Gpio<GPIOA_BASE,9>::mode(Mode::INPUT_PULL_UP_DOWN); //Serial port
    //Gpio<GPIOA_BASE,10>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOA_BASE,11>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOA_BASE,12>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOA_BASE,13>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOA_BASE,14>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOA_BASE,15>::mode(Mode::INPUT_PULL_UP_DOWN);
    
    //Gpio<GPIOB_BASE,0>::mode(Mode::INPUT_PULL_UP_DOWN); //Nrf irq
    //Gpio<GPIOB_BASE,1>::mode(Mode::INPUT_PULL_UP_DOWN); //RTC clock in
    Gpio<GPIOB_BASE,2>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,3>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,4>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,5>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,6>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,7>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,8>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,9>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,10>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,11>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,12>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,13>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,14>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,15>::mode(Mode::INPUT_PULL_UP_DOWN);
    
    Gpio<GPIOC_BASE,0>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOC_BASE,1>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOC_BASE,2>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOC_BASE,3>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOC_BASE,4>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOC_BASE,5>::mode(Mode::INPUT_PULL_UP_DOWN);
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

#endif //LOW_POWER_SETUP_H
