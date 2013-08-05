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

#include <cassert>
#include <miosix.h>
#include <miosix/kernel/scheduler/scheduler.h>
#include "rtc.h"

using namespace miosix;

#ifdef _BOARD_STM32VLDISCOVERY

static Thread *waiting=0;                ///< Thread waiting for the RTC interrupt
static volatile bool rtcInterrupt=false; ///< The RTC interrupt has fired
static Thread *timeoutWaiting=0;         ///< Thread waiting for the receiveWithTimeout
static volatile bool timeout=false;        ///< A timeout happened
static volatile bool packetReceived=false; ///< A packet was received
static void (*eventHandler)(unsigned int)=0; ///< Called when event received
static Thread *vhtWaiting;               ///< Thread waiting on VHT
static unsigned int vhtSyncPointRtc;     ///< Rtc time corresponding to vht time
static unsigned int vhtSyncPointVht;     ///< Vht time corresponding to rtc time
static bool vhtTimeout=false;            ///< VHT timed out

typedef Gpio<GPIOC_BASE,13> clockout;
typedef Gpio<GPIOB_BASE,1> clockin;

/**
 * RTC interrupt
 */
void __attribute__((naked)) RTC_IRQHandler()
{
	saveContext();
	asm volatile("bl _Z14RTChandlerImplv");
	restoreContext();
}

/**
 * RTC interrupt actual implementation
 */
void __attribute__((used)) RTChandlerImpl()
{
    RTC->CRL &= ~RTC_CRL_ALRF;
    EXTI->PR=EXTI_PR_PR17;
    rtcInterrupt=true;
    if(!waiting) return;
    waiting->IRQwakeup();
	if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		Scheduler::IRQfindNextThread();
    waiting=0;
}

/**
 * EXTI0 is connected to the NRF24L01+ IRQ pin, so this interrupt is fired
 * on an IRQ falling edge
 */
void __attribute__((naked)) EXTI0_IRQHandler()
{
	saveContext();
	asm volatile("bl _Z17nrfIrqhandlerImplv");
	restoreContext();
}

/**
 * NRF24L01+ IRQ actual implementation
 */
void __attribute__((used)) nrfIrqhandlerImpl()
{
    EXTI->PR=EXTI_PR_PR0;
    packetReceived=true;
    if(!timeoutWaiting) return;
    timeoutWaiting->IRQwakeup();
	if(timeoutWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		Scheduler::IRQfindNextThread();
    timeoutWaiting=0;
}

/**
 * EXTI1 is the event input, for event timestamping
 */
void EXTI1_IRQHandler()
{
    EXTI->PR=EXTI_PR_PR1;

    //Get a timestamp of the event
    unsigned int a,b;
    do {
        a=RTC->CNTL;
        b=RTC->CNTH;
    } while(a!=RTC->CNTL); //Ensure no updates in the middle

    if(eventHandler) eventHandler(a | b<<16);
}

/**
 * TIM7 is the timeout timer
 */
void __attribute__((naked)) TIM7_IRQHandler()
{
	saveContext();
	asm volatile("bl _Z15tim7handlerImplv");
	restoreContext();
}

/**
 * TIM7 irq actual implementation
 */
void __attribute__((used)) tim7handlerImpl()
{
    TIM7->SR=0;
    timeout=true;
    if(!timeoutWaiting) return;
    timeoutWaiting->IRQwakeup();
	if(timeoutWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		Scheduler::IRQfindNextThread();
    timeoutWaiting=0;
}

/**
 * TIM3 is the vht timer
 */
void __attribute__((naked)) TIM3_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15tim3handlerImplv");
    restoreContext();
}

/**
 * TIM3 irq actual implementation
 */
void __attribute__((used)) tim3handlerImpl()
{
    if(TIM3->SR & TIM_SR_CC4IF) vhtSyncPointVht=TIM3->CCR4;
    if(TIM3->SR & TIM_SR_UIF) vhtTimeout=true;
    TIM3->SR=0;
    if(!vhtWaiting) return;
    vhtWaiting->IRQwakeup();
    if(vhtWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    vhtWaiting=0;
}

//
// class AuxiliaryTimer
//

AuxiliaryTimer& AuxiliaryTimer::instance()
{
    static AuxiliaryTimer timer;
    return timer;
}

void AuxiliaryTimer::initTimeoutTimer(unsigned short ctr)
{
    packetReceived=false;
    timeout=false;
    if(ctr==0) return;
    TIM7->ARR=ctr;
    TIM7->CR1 |= TIM_CR1_CEN;
}

bool AuxiliaryTimer::waitForPacketOrTimeout()
{
    FastInterruptDisableLock dLock;
    while(packetReceived==false && timeout==false)
    {
        timeoutWaiting=Thread::IRQgetCurrentThread();
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
    packetReceived=false;
    return timeout;
}

void AuxiliaryTimer::waitForTimeout()
{
    FastInterruptDisableLock dLock;
    while(timeout==false)
    {
        timeoutWaiting=Thread::IRQgetCurrentThread();
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
}

void AuxiliaryTimer::stopTimeoutTimer()
{
    TIM7->CR1=TIM_CR1_OPM;
    TIM7->CNT=0;
}

AuxiliaryTimer::AuxiliaryTimer()
{
    {
        FastInterruptDisableLock dLock;
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
        RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
        AFIO->EXTICR[0] |= AFIO_EXTICR1_EXTI0_PB;
        EXTI->IMR |= EXTI_IMR_MR0;
        EXTI->FTSR |= EXTI_FTSR_TR0;
    }
    TIM7->CR1=TIM_CR1_OPM;
    TIM7->DIER=TIM_DIER_UIE;
    TIM7->PSC=24000000/16384-1;
    TIM7->CNT=0;
    NVIC_ClearPendingIRQ(EXTI0_IRQn);
    NVIC_SetPriority(EXTI0_IRQn,10); //Low priority
	NVIC_EnableIRQ(EXTI0_IRQn); 
    NVIC_ClearPendingIRQ(TIM7_IRQn);
    NVIC_SetPriority(TIM7_IRQn,10); //Low priority
	NVIC_EnableIRQ(TIM7_IRQn);
}

//
// class HardwareTimer
//

Rtc& Rtc::instance()
{
    static Rtc timer;
    return timer;
}

unsigned int Rtc::getValue() const
{
    unsigned int a,b;
    do {
        a=RTC->CNTL;
        b=RTC->CNTH;
    } while(a!=RTC->CNTL); //Ensure no updates in the middle
    return a | b<<16;
}

unsigned int Rtc::getPacketTimestamp() const
{
    //Same as getValue(), inlining code to avoid an additional function call overhead
    unsigned int a,b;
    do {
        a=RTC->CNTL;
        b=RTC->CNTH;
    } while(a!=RTC->CNTL); //Ensure no updates in the middle
    return a | b<<16;
}

void Rtc::setAbsoluteWakeupWait(unsigned int value)
{
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRL=value & 0xffff;
    RTC->ALRH=value>>16;
    RTC->CRL &= ~RTC_CRL_CNF;
    rtcInterrupt=false;
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
}

void Rtc::setRelativeWakeup(unsigned int value)
{
    setAbsoluteWakeupWait(getValue()+value);
}

void Rtc::wait()
{
    FastInterruptDisableLock dLock;
    while(rtcInterrupt==false)
    {
        waiting=Thread::IRQgetCurrentThread();
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
}

void Rtc::setAbsoluteWakeupSleep(unsigned int value)
{
    setAbsoluteWakeupWait(value); //For RTC there's no difference
}

void Rtc::sleep()
{
    while(Console::txComplete()==false) ;

    {
        FastInterruptDisableLock dLock;
        if(rtcInterrupt) return;

        sleepAgain:
        PWR->CR |= PWR_CR_LPDS;
        SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
        
        // Using WFE instead of WFI because if while we are with interrupts
        // disabled an interrupt (such as the tick interrupt) occurs, it
        // remains pending and the WFI becomes a nop, and the device never goes
        // in sleep mode. WFE events are latched in a separate pending register
        // so interrupts do not interfere with them
        __WFE();
        
        SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
        PWR->CR &= ~PWR_CR_LPDS;

        //If an event occurs while the node is in deep sleep, instead of waiting
        //for the PLL to startup and for interrupts to be enabled again, which
        //would take too long, duplicate here the code of the interrupt, and
        //disable the interrupt pending bit
        if(NVIC_GetPendingIRQ(EXTI1_IRQn))
        {
            //Did you know that reading the RTC registers less than ~32us after
            //wakeup has a high probability of returning garbage? Neither did I,
            //as this FUCKING QUIRK ISN'T DOCUMENTED!
            //Since we have to wait, wait an entire 61us or 1 timer tick, and
            //then subtract 1 to the result of getValue(). Now, as we're running
            //@8MHz but Miosix delay loops are calibrated for 24, the parameter
            //passed to delayUs is 61/24*8=20
            delayUs(20);
            if(eventHandler) eventHandler(getValue()-1);
            EXTI->PR=EXTI_PR_PR1;
            NVIC_ClearPendingIRQ(EXTI1_IRQn);
        }

        //If the wakeup was just due to the event go back sleeping
        if(NVIC_GetPendingIRQ(RTC_IRQn)==0) goto sleepAgain;
        
        //STOP mode resets the clock to the HSI 8MHz, so restore the 24MHz clock
        RCC->CR |= RCC_CR_HSEON;
        while((RCC->CR & RCC_CR_HSERDY)==0) ;
        RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
        RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;
        RCC->CFGR |= RCC_CFGR_PPRE1_DIV1;
        //PLL configuration:  = (HSE / 2) * 6 = 24 MHz
        RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
        RCC->CFGR |= RCC_CFGR_PLLSRC_PREDIV1
                   | RCC_CFGR_PLLXTPRE_PREDIV1_Div2
                   | RCC_CFGR_PLLMULL6;

        RCC->CR |= RCC_CR_PLLON;
        while((RCC->CR & RCC_CR_PLLRDY)==0) ;
        RCC->CFGR &= ~RCC_CFGR_SW;
        RCC->CFGR |= RCC_CFGR_SW_PLL;    
        while((RCC->CFGR & RCC_CFGR_SWS)!=0x08) ; 
    }
}

Rtc::Rtc()
{
    FastInterruptDisableLock dLock;
    clockin::mode(Mode::INPUT);
    clockout::mode(Mode::OUTPUT);
    RCC->APB1ENR |= RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_DBP;
    RCC->BDCR |= RCC_BDCR_LSEON     //Enable 32KHz oscillator
               | RCC_BDCR_RTCEN     //Enable RTC
               | RCC_BDCR_RTCSEL_0; //Connect the two together
    while((RCC->BDCR & RCC_BDCR_LSERDY)==0) ; //Wait
    RTC->CRH |= RTC_CRH_ALRIE;
    RTC->CRL |= RTC_CRL_CNF;
    RTC->PRLH=0;
    RTC->PRLL=1; //Divide by 2, clock is 16384Hz
    RTC->CNTL=0;
    RTC->CNTH=0;
    RTC->ALRL=0xffff;
    RTC->ALRH=0xffff;
    RTC->CRL &= ~RTC_CRL_CNF;
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
    EXTI->IMR |= EXTI_IMR_MR17;
    EXTI->EMR |= EXTI_EMR_MR17;
    EXTI->RTSR |= EXTI_RTSR_TR17;
    EXTI->PR=EXTI_PR_PR17; //Clear eventual pending IRQ
    BKP->RTCCR=BKP_RTCCR_ASOS | BKP_RTCCR_ASOE; //Enable RTC clock out
    NVIC_SetPriority(RTC_IRQn,10); //Low priority
	NVIC_EnableIRQ(RTC_IRQn);
}

void setEventHandler(void (*handler)(unsigned int))
{
    Rtc::instance(); //Ensure the RTC is initialized
    {
        FastInterruptDisableLock dLock;
        eventHandler=handler;

        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
        AFIO->EXTICR[0] |= AFIO_EXTICR1_EXTI1_PB;
        EXTI->IMR |= EXTI_IMR_MR1;
        EXTI->EMR |= EXTI_EMR_MR1;
        EXTI->RTSR |= EXTI_RTSR_TR1;
        EXTI->PR=EXTI_PR_PR1; //Clear eventual pending IRQ
    }
    NVIC_ClearPendingIRQ(EXTI1_IRQn);
    NVIC_SetPriority(EXTI1_IRQn,2); //High priority
    NVIC_EnableIRQ(EXTI1_IRQn); 
}

//
// class VHT
//

VHT& VHT::instance()
{
    static VHT timer;
    return timer;
}

unsigned int VHT::getValue() const
{
    //Unfortunately the timer is only 16 bits, so when waiting long
    //in resynchronize it times out. In this case fallback to rtc
    if(vhtTimeout)
    {
        unsigned long long conversion=rtc.getValue();
        conversion*=1000000;
        conversion+=16384/2; //Round to nearest
        conversion/=16384;
        return conversion;
    } else return TIM3->CNT-vhtSyncPointVht+vhtBase;
}

unsigned int VHT::getPacketTimestamp() const
{
    //Unfortunately the timer is only 16 bits, so when waiting long
    //in resynchronize it times out. In this case fallback to rtc
    if(vhtTimeout)
    {
        unsigned long long conversion=rtc.getValue();
        conversion*=1000000;
        conversion+=16384/2; //Round to nearest
        conversion/=16384;
        return conversion;
    } else return TIM3->CCR3-vhtSyncPointVht+vhtBase;
}

void VHT::setAbsoluteWakeupWait(unsigned int value)
{
    unsigned int t=value-vhtBase+vhtSyncPointVht;
    assert(t<0xffff);
    TIM3->CCR1=t;
}

void VHT::wait()
{
    FastInterruptDisableLock dLock;
    TIM3->DIER |= TIM_DIER_CC1IE;
    vhtWaiting=Thread::IRQgetCurrentThread();
    while(vhtWaiting)
    {
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
    TIM3->DIER &=~ TIM_DIER_CC1IE;
}

void VHT::setAbsoluteWakeupSleep(unsigned int value)
{
    if(value<last) overflows++;
    last=value;
    //Unfortunately on the stm32vldiscovery the rtc runs at 16384,
    //while the other timer run at a submultiple of 24MHz, 1MHz in
    //the current setting, and since 1MHz is not a multiple of 16384
    //the conversion is a little complex and requires the use of
    //64bit numbers for intermendiate results. If the main XTAL was
    //8.388608MHz instead of 8MHz a simple shift operation on 32bit
    //numbers would suffice.
    unsigned long long conversion=overflows;
    conversion<<=32;
    conversion|=value;
    conversion*=16384;
    conversion+=1000000/2; //Round to nearest
    conversion/=1000000;
    unsigned int rtcTime=conversion;
    rtc.setAbsoluteWakeupSleep(rtcTime);
}

void VHT::sleep()
{
    rtc.sleep();
    synchronizeWithRtc();
}

void VHT::synchronizeWithRtc()
{
    {
        FastInterruptDisableLock dLock;
        TIM3->CNT=0;
        TIM3->EGR=TIM_EGR_UG;
        //These two wait make sure the next code is run exactly after
        //the falling edge of the 16KHz clock
        while(clockin::value()==0) ;
        while(clockin::value()==1) ;
        //This dely is extremely important. There appears to be some
        //kind of resynchronization between clock domains happening
        //and reading the RTC registers immediately after the falling
        //edge sometimes returns the old value, and sometimes the new
        //one. This jitter is unacceptable, and the delay fixes it.
        delayUs(18);
        //Get RTC value
        unsigned int a,b;
        do {
            a=RTC->CNTL;
            b=RTC->CNTH;
        } while(a!=RTC->CNTL); //Ensure no updates in the middle
        vhtSyncPointRtc=a | b<<16;
        TIM3->DIER |= TIM_DIER_CC4IE;
        TIM3->CCER |= TIM_CCER_CC4E; //Enable capture on channel4
        vhtWaiting=Thread::IRQgetCurrentThread();
        while(vhtWaiting)
        {
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
        TIM3->DIER &=~ TIM_DIER_CC4IE;
        TIM3->CCER &=~ TIM_CCER_CC4E; //Disable capture on channel4
        vhtTimeout=false;
    }
    //Unfortunately on the stm32vldiscovery the rtc runs at 16384,
    //while the other timer run at a submultiple of 24MHz, 1MHz in
    //the current setting, and since 1MHz is not a multiple of 16384
    //the conversion is a little complex and requires the use of
    //64bit numbers for intermendiate results. If the main XTAL was
    //8.388608MHz instead of 8MHz a simple bitmask operation on 32bit
    //numbers would suffice.
    unsigned long long conversion=vhtSyncPointRtc;
    conversion*=1000000;
    conversion+=16384/2; //Round to nearest
    conversion/=16384;
    vhtBase=conversion;
}

VHT::VHT() : rtc(Rtc::instance()), vhtBase(0), last(0), overflows(0)
{
    {
        FastInterruptDisableLock dLock;
        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    }
    
    TIM3->CNT=0;
    TIM3->PSC=24000000/1000000-1; //High frequency timer runs @ 1MHz
    TIM3->CCMR2=TIM_CCMR2_CC3S_0  //CC3 connected to input 3 (nRF IRQ)
              | TIM_CCMR2_IC3F_0  //Sample at 24MHz, resynchronize with 2 samples
              | TIM_CCMR2_CC4S_0  //CC4 connected to input 4 (rtc freq for resync)
              | TIM_CCMR2_IC4F_0; //Sample at 24MHz, resynchronize with 2 samples
    TIM3->CCER=TIM_CCER_CC3P  //Capture nRF IRQ on falling edge
             | TIM_CCER_CC3E; //Capture enabled for nRF IRQ
    TIM3->DIER=TIM_DIER_UIE;  //Enable interrupt @ end of time to set flag
    TIM3->CR1=TIM_CR1_CEN;
    NVIC_SetPriority(TIM3_IRQn,10); //Low priority
    NVIC_ClearPendingIRQ(TIM3_IRQn);
    NVIC_EnableIRQ(TIM3_IRQn);
    
    synchronizeWithRtc();
}

#else //_BOARD_STM32VLDISCOVERY
#error "rtc.cpp not implemented for the selected board"
#endif //_BOARD_STM32VLDISCOVERY
