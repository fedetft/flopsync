
#ifndef BOARD_SETUP_H
#define BOARD_SETUP_H

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
    
    Gpio<GPIOB_BASE,0>::mode(Mode::INPUT_PULL_UP_DOWN); 
    Gpio<GPIOB_BASE,1>::mode(Mode::INPUT_PULL_UP_DOWN); 
    //Gpio<GPIOB_BASE,2>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 fifop_irq
    Gpio<GPIOB_BASE,3>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,4>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,5>::mode(Mode::INPUT_PULL_UP_DOWN); 
    //Gpio<GPIOB_BASE,6>::mode(Mode::INPUT_PULL_UP_DOWN); //TIM4 ch1 vhtSyncRtc
    Gpio<GPIOB_BASE,7>::mode(Mode::INPUT_PULL_UP_DOWN);
    //Gpio<GPIOB_BASE,8>::mode(Mode::INPUT_PULL_UP_DOWN); //TIM4 ch3 wait
    //Gpio<GPIOB_BASE,9>::mode(Mode::INPUT_PULL_UP_DOWN); //TIM4 ch4 trigger
    Gpio<GPIOB_BASE,10>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,11>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,12>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,13>::mode(Mode::INPUT_PULL_UP_DOWN); //debugger
    Gpio<GPIOB_BASE,14>::mode(Mode::INPUT_PULL_UP_DOWN); //debbuger
    Gpio<GPIOB_BASE,15>::mode(Mode::INPUT_PULL_UP_DOWN);
    
    //Gpio<GPIOC_BASE,0>::mode(Mode::INPUT_PULL_UP_DOWN); //ADC TMP35
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

typedef miosix::Gpio<GPIOA_BASE,11> probe_int_dis;
typedef miosix::Gpio<GPIOB_BASE,10> probe_wakeup;
typedef miosix::Gpio<GPIOB_BASE,11> probe_pll_boot;
typedef miosix::Gpio<GPIOB_BASE,12> probe_sync_vht;
typedef miosix::Gpio<GPIOB_BASE,13> probe_xosc_radio_boot;
typedef miosix::Gpio<GPIOB_BASE,14> probe_pin14;
typedef miosix::Gpio<GPIOB_BASE,15> probe_pin15;
typedef miosix::Gpio<GPIOA_BASE,0> button;

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
    //Gpio<GPIOA_BASE,11>::mode(Mode::INPUT_PULL_UP_DOWN); //debug int_dis
    Gpio<GPIOA_BASE,12>::mode(Mode::INPUT_PULL_UP_DOWN); 
    Gpio<GPIOA_BASE,13>::mode(Mode::INPUT_PULL_UP_DOWN); 
    Gpio<GPIOA_BASE,14>::mode(Mode::INPUT_PULL_UP_DOWN); 
    Gpio<GPIOA_BASE,15>::mode(Mode::INPUT_PULL_UP_DOWN); 
    
    Gpio<GPIOB_BASE,0>::mode(Mode::INPUT_PULL_UP_DOWN); 
    Gpio<GPIOB_BASE,1>::mode(Mode::INPUT_PULL_UP_DOWN); 
    //Gpio<GPIOB_BASE,2>::mode(Mode::INPUT_PULL_UP_DOWN); //cc2520 fifop_irq
    Gpio<GPIOB_BASE,3>::mode(Mode::INPUT_PULL_UP_DOWN);
    Gpio<GPIOB_BASE,4>::mode(Mode::INPUT_PULL_UP_DOWN); 
    Gpio<GPIOB_BASE,5>::mode(Mode::INPUT_PULL_UP_DOWN); 
    //Gpio<GPIOB_BASE,6>::mode(Mode::INPUT_PULL_UP_DOWN); //TIM4 ch1 vhtSyncRtc
    Gpio<GPIOB_BASE,7>::mode(Mode::INPUT_PULL_UP_DOWN);
    
    //Gpio<GPIOB_BASE,8>::mode(Mode::INPUT_PULL_UP_DOWN); //TIM4 ch3 wait
    //Gpio<GPIOB_BASE,9>::mode(Mode::INPUT_PULL_UP_DOWN); //TIM4 ch4 trigger
    
    //Gpio<GPIOB_BASE,10>::mode(Mode::INPUT_PULL_UP_DOWN); //debug
    //Gpio<GPIOB_BASE,11>::mode(Mode::INPUT_PULL_UP_DOWN); //debug
    //Gpio<GPIOB_BASE,12>::mode(Mode::INPUT_PULL_UP_DOWN); //debug
    //Gpio<GPIOB_BASE,13>::mode(Mode::INPUT_PULL_UP_DOWN); //debug
    //Gpio<GPIOB_BASE,14>::mode(Mode::INPUT_PULL_UP_DOWN); //debug
    //Gpio<GPIOB_BASE,15>::mode(Mode::INPUT_PULL_UP_DOWN); //debug
    
    Gpio<GPIOC_BASE,0>::mode(Mode::INPUT_PULL_UP_DOWN); //ADC TMP35
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
    
    button::mode(miosix::Mode::OUTPUT);
    probe_int_dis::mode(miosix::Mode::OUTPUT);
    probe_wakeup::mode(miosix::Mode::OUTPUT);
    probe_pll_boot::mode(miosix::Mode::OUTPUT);
    probe_sync_vht::mode(miosix::Mode::OUTPUT);
    probe_xosc_radio_boot::mode(miosix::Mode::OUTPUT);
    probe_pin14::mode(miosix::Mode::OUTPUT);
    probe_pin15::mode(miosix::Mode::OUTPUT);
}

#endif //FLOPSYNC_DEBUG
#endif //BOARD_SETUP_H
