
#ifndef LOW_POWER_SETUP_H
#define LOW_POWER_SETUP_H

#include <miosix.h>
#include "flopsync_v3/protocol_constants.h"

#if FLOPSYNC_DEBUG == 0
//inline because it's called once
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
    
    //Gpio<GPIOB_BASE,0>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 GPIO3
    //Gpio<GPIOB_BASE,1>::mode(Mode::INPUT_PULL_UP_DOWN); //send packet
    //Gpio<GPIOB_BASE,2>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 fifop_irq
    Gpio<GPIOB_BASE,3>::mode(Mode::INPUT_PULL_UP_DOWN);
    //Gpio<GPIOB_BASE,4>::mode(Mode::INPUT_PULL_UP_DOWN); //RTC clock in
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
    //Gpio<GPIOC_BASE,4>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 fifo
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
#else 
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
    //Gpio<GPIOA_BASE,11>::mode(Mode::INPUT_PULL_UP_DOWN); //hw debug wakeup
    //Gpio<GPIOA_BASE,12>::mode(Mode::INPUT_PULL_UP_DOWN); //hw debug pll
    Gpio<GPIOA_BASE,13>::mode(Mode::INPUT_PULL_UP_DOWN); 
    Gpio<GPIOA_BASE,14>::mode(Mode::INPUT_PULL_UP_DOWN); 
    Gpio<GPIOA_BASE,15>::mode(Mode::INPUT_PULL_UP_DOWN); 
    
    //Gpio<GPIOB_BASE,0>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 GPIO3
    //Gpio<GPIOB_BASE,1>::mode(Mode::INPUT_PULL_UP_DOWN); //send packet
    //Gpio<GPIOB_BASE,2>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 fifop_irq
    Gpio<GPIOB_BASE,3>::mode(Mode::INPUT_PULL_UP_DOWN);
    //Gpio<GPIOB_BASE,4>::mode(Mode::INPUT_PULL_UP_DOWN); //RTC clock in
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
    //Gpio<GPIOC_BASE,4>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 fifo
    //Gpio<GPIOC_BASE,5>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 sfd 
    Gpio<GPIOC_BASE,6>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOC_BASE,7>::mode(Mode::INPUT_PULL_UP_DOWN);
    //Gpio<GPIOC_BASE,8>::mode(Mode::INPUT_PULL_UP_DOWN); //Led
    //Gpio<GPIOC_BASE,9>::mode(Mode::INPUT_PULL_UP_DOWN); //Led
    //Gpio<GPIOC_BASE,10>::mode(Mode::INPUT_PULL_UP_DOWN); //hw debug syncVHT
    //Gpio<GPIOC_BASE,11>::mode(Mode::INPUT_PULL_UP_DOWN); //hw debug xoscRadioBoot
    //Gpio<GPIOC_BASE,12>::mode(Mode::INPUT_PULL_UP_DOWN); //hw debug jitterHW
    //Gpio<GPIOC_BASE,13>::mode(Mode::INPUT_PULL_UP_DOWN); //RTC clock out
    //Gpio<GPIOC_BASE,14>::mode(Mode::INPUT_PULL_UP_DOWN); //32KHz XTAL
    //Gpio<GPIOC_BASE,15>::mode(Mode::INPUT_PULL_UP_DOWN);
}
#endif //FLOPSYNC_DEBUG
#endif //LOW_POWER_SETUP_H
