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
#include <../miosix/kernel/scheduler/scheduler.h>
#include "timer.h"

using namespace miosix;

#ifdef _BOARD_STM32VLDISCOVERY


static Thread *rtcWaiting=0;        ///< Thread waiting for the RTC interrupt
static Thread *vhtWaiting=0;        ///< Thread waiting on VHT

static volatile typeVecInt rtcInt;

static volatile typeVecInt vhtInt;

static volatile bool rtcTriggerEnable=false;
static void (*eventHandler)(unsigned int)=0; ///< Called when event received
static unsigned long long vhtSyncPointRtc=0;     ///< Rtc time corresponding to vht time
static volatile unsigned long long vhtSyncPointVht=0;     ///< Vht time corresponding to rtc time
static volatile unsigned long long vhtOverflows=0;  //< counter VHT overflows
static volatile unsigned long long timestampEvent=0; ///< input capture timestamp
static unsigned int rtcOverflows=0;        ///< counter RTC overflows
static unsigned int rtcLast=0;             ///< variable for evaluate overflow
static unsigned long long vhtWakeupWait=0;

typedef Gpio<GPIOB_BASE,9> trigger;
typedef Gpio<GPIOC_BASE,13> clockout;
typedef Gpio<GPIOB_BASE,6> clockin;

#if TIMER_DEBUG >0
typedef miosix::Gpio<GPIOA_BASE,11> wakeup;
typedef miosix::Gpio<GPIOA_BASE,12> pll;
typedef miosix::Gpio<GPIOC_BASE,10> syncVht;
#endif//TIMER_DEBUG

#if TIMER_DEBUG ==4
typeTimer info;
#endif //TIMER_DEBUG

static unsigned long long inline getRTC64bit(unsigned int rtcCounter)
{
    if(rtcCounter<rtcLast) rtcOverflows++;
    rtcLast=rtcCounter;
    unsigned long long result = rtcOverflows;
    result <<= 32;
    result |= rtcCounter;
    return result;
}


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
    RTC->CRL =~RTC_CRL_ALRF;
    EXTI->PR=EXTI_PR_PR17;
    rtcInt.wait=true;
    if(rtcTriggerEnable)
    {
        trigger::high();
        rtcTriggerEnable=false;
        rtcInt.trigger=true;
        RTC->CRL &=~RTC_CRH_ALRIE;
    }
    if(!rtcWaiting) return;
    rtcWaiting->IRQwakeup();
	if(rtcWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		Scheduler::IRQfindNextThread();
    rtcWaiting=0;
}

/**
 * EXTI0 is connected to the cc2520 SFD pin, so this interrupt is fired
 * on an IRQ falling edge
 */
void __attribute__((naked)) EXTI0_IRQHandler()
{
	saveContext();
	asm volatile("bl _Z20cc2520IrqhandlerImplv");
	restoreContext();
}

/**
 * cc2520 SFD actual implementation
 */
void __attribute__((used)) cc2520IrqhandlerImpl()
{
    EXTI->PR=EXTI_PR_PR0;
    rtcInt.event=true;
    if(!rtcWaiting) return;
    rtcWaiting->IRQwakeup();
	if(rtcWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		Scheduler::IRQfindNextThread();
    rtcWaiting=0;
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
 * TIM4 is the vht timer
 */
void __attribute__((naked)) TIM4_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15tim4handlerImplv");
    restoreContext();
}

/**
 * TIM4 irq actual implementation
 */
void __attribute__((used)) tim4handlerImpl()
{ 
    //vhtSyncvht input capture channel 1
    if((TIM4->SR & TIM_SR_CC1IF) && (TIM4->DIER & TIM_DIER_CC1IE))
    {
        TIM4->SR =~TIM_SR_CC1IF;
        TIM4->DIER&=~TIM_DIER_CC1IE;
        vhtInt.sync=true;
        unsigned short timeCapture=TIM4->CCR1;
        vhtSyncPointVht=vhtOverflows|timeCapture;
        //check if overflow interrupt happened before IC1 interrupt
        if((TIM4->SR & TIM_SR_UIF) && (timeCapture <= TIM4->CNT))
            vhtSyncPointVht+=1<<16;
    }
    
    
    //VHT wait output compare channel 2
    if((TIM4->SR & TIM_SR_CC2IF) && (TIM4->DIER & TIM_DIER_CC2IE))
    {
        TIM4->SR =~TIM_SR_CC2IF;
        //check if overflow interrupt happened before OC2 interrupt
        bool ovf=(TIM4->SR & TIM_SR_UIF) && (TIM4->CCR2 <= TIM4->CNT);
        //<= is necessary for allow wakeup missed
        if((vhtWakeupWait & 0xFFFFFFFFFFFF0000) <= (vhtOverflows+(ovf?1<<16:0)))
        {    
            vhtInt.wait=true;
        }
        
    }
    
    //IRQ input capture channel 3
    if((TIM4->SR & TIM_SR_CC3IF) && (TIM4->DIER & TIM_DIER_CC3IE))
    {
        #if TIMER_DEBUG ==4
        info.ts=vhtOverflows|TIM4->CCR3;
        info.ovf= vhtOverflows;
        info.sr = TIM4->SR;
        #endif //TIMER_DEBUG
        TIM4->SR =~TIM_SR_CC3IF;
        vhtInt.event=true;
        unsigned short timeCapture=TIM4->CCR3;
        timestampEvent=vhtOverflows|timeCapture;
      
        #if TIMER_DEBUG ==4
        info.cnt=TIM4->CNT;
        #endif //TIMER_DEBUG
        //check if overflow interrupt happened before IC3 interrupt
        if((TIM4->SR & TIM_SR_UIF) && (timeCapture <= TIM4->CNT))
            timestampEvent+=1<<16;
        
        #if TIMER_DEBUG ==4
        info.cntFirstUIF=TIM4->CNT;
        info.srFirstUIF=TIM4->SR;
        #endif //TIMER_DEBUG
     }
    
    //Send packet output compare channel 4
    if((TIM4->SR & TIM_SR_CC4IF) && (TIM4->DIER & TIM_DIER_CC4IE))
    {
        TIM4->SR =~TIM_SR_CC4IF;
        //calculating the remaining slots
        long long remainingSlot = vhtWakeupWait -
                            (vhtOverflows+(((TIM4->SR & TIM_SR_UIF) && (TIM4->CCR4 <= TIM4->CNT))?1<<16:0));
        remainingSlot >>=16;
        
        //<= is necessary for allow wakeup missed
        if(remainingSlot <= 0)    
        {
            TIM4->DIER &=~TIM_DIER_CC4IE;
            TIM4->CCMR2 &=~TIM_CCMR2_OC4M_0 ;  //reset oc on mathc
            TIM4->CCMR2 |=TIM_CCMR2_OC4M_2 ;   //force inactive level
            vhtInt.trigger=true;
            TIM4->CCER &=~TIM_CCER_CC4E;
            TIM4->CCMR2 &=~TIM_CCMR2_OC4M_2 ; //reset force inactive level
            TIM4->CCMR2 |=TIM_CCMR2_OC4M_0 ;  //set oc on match
        }
        else if(remainingSlot==1)
                TIM4->CCER |= TIM_CCER_CC4E;
    }
    
    //IRQ overflow
    if(TIM4->SR & TIM_SR_UIF)
    {
        TIM4->SR =~TIM_SR_UIF;
        vhtOverflows+=1<<16;
    }
    
    if((!vhtInt.event && !vhtInt.sync && !vhtInt.trigger && !vhtInt.wait ) || !vhtWaiting) return;
    vhtWaiting->IRQwakeup();
    if(vhtWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    vhtWaiting=0;
}

//
// class Rtc
//

Rtc& Rtc::instance()
{
    static Rtc timer;
    return timer;
}

unsigned long long Rtc::getValue() const
{
    unsigned int a,b;
    do {
        a=RTC->CNTL;
        b=RTC->CNTH;
    } while(a!=RTC->CNTL); //Ensure no updates in the middle
    return getRTC64bit(a | b<<16);
    
}

void Rtc::setValue(unsigned long long value)
{
    rtcOverflows=value>>32;
    RTC->CRL |= RTC_CRL_CNF;
    RTC->CNTL=value & 0xffff;
    RTC->CNTH=value>>16 ;
    RTC->CRL &= ~RTC_CRL_CNF;
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
}

unsigned long long Rtc::getExtEventTimestamp() const
{
    //Same as getValue(), inlining code to avoid an additional function call overhead
    unsigned int a,b;
    do {
        a=RTC->CNTL;
        b=RTC->CNTH;
    } while(a!=RTC->CNTL); //Ensure no updates in the middle
    return getRTC64bit(a | b<<16);
}

void Rtc::absoluteWait(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    RTC->CRL =~RTC_CRL_ALRF;
    RTC->CRH |= RTC_CRH_ALRIE;
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRL=value & 0xffff;
    RTC->ALRH=value>>16;
    RTC->CRL &= ~RTC_CRL_CNF;
    rtcInt.wait=false;
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
    if(value > getValue())
    {
        while(!rtcInt.wait)
        {
            rtcWaiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    }
    RTC->CRH &=~ RTC_CRH_ALRIE;
}

bool Rtc::absoluteWaitTimeoutOrEvent(unsigned long long value)
{
    if(value)
    {
        FastInterruptDisableLock dLock;
        RTC->CRL |= RTC_CRL_CNF;
        RTC->CRL =~RTC_CRL_ALRF;
        RTC->CRH |= RTC_CRH_ALRIE;
        RTC->ALRL=value & 0xffff;
        RTC->ALRH=value>>16;
        RTC->CRL &= ~RTC_CRL_CNF;
        rtcInt.wait=false;
        rtcInt.trigger=false;
        while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
        if(value > getValue())
        {
            while(!rtcInt.wait && !rtcInt.event)
            {
                rtcWaiting=Thread::IRQgetCurrentThread();
                Thread::IRQwait();
                {
                    FastInterruptEnableLock eLock(dLock);
                    Thread::yield();
                }
            }
            RTC->CRH &=~RTC_CRH_ALRIE;
        }
    }
    else
    {
        FastInterruptDisableLock dLock;
        rtcInt.event=false;
        while(!rtcInt.event)
        {
            rtcWaiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    }
    return rtcInt.wait;
}

void Rtc::absoluteTriggerEvent(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    trigger::low();
    RTC->CRL =~RTC_CRL_ALRF;
    RTC->CRH |= RTC_CRH_ALRIE;
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRL=value & 0xffff;
    RTC->ALRH=value>>16;
    RTC->CRL &= ~RTC_CRL_CNF;
    rtcTriggerEnable=true;
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
    if(value <= getValue()) 
        rtcTriggerEnable=false;
    FastInterruptEnableLock eLock(dLock);
}

void Rtc::absoluteWaitTriggerEvent(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    trigger::low();
    RTC->CRL =~RTC_CRL_ALRF;
    RTC->CRH |= RTC_CRH_ALRIE;
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRL=value & 0xffff;
    RTC->ALRH=value>>16;
    RTC->CRL &= ~RTC_CRL_CNF;
    rtcInt.trigger=false;
    rtcTriggerEnable=true;
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
    if(value > getValue())
    {    
        while(!rtcInt.trigger)
        {
            rtcWaiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    }
    rtcTriggerEnable=false;
    RTC->CRH &=~RTC_CRH_ALRIE;
}

void Rtc::wait(unsigned long long value)
{
    absoluteWait(getValue()+value);
}

void Rtc::absoluteSleep(unsigned long long value)
{
    RTC->CRL =~RTC_CRL_ALRF;
    RTC->CRH |= RTC_CRH_ALRIE;
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRL=value & 0xffff;
    RTC->ALRH=value>>16;
    RTC->CRL &= ~RTC_CRL_CNF;
    rtcInt.wait=false;
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait

    while(Console::txComplete()==false) ;
    {
        FastInterruptDisableLock dLock;
        if(rtcInt.wait) return;
        
        PWR->CR |= PWR_CR_LPDS;
        SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
        
        // Using WFE instead of WFI because if while we are with interrupts
        // disabled an interrupt (such as the tick interrupt) occurs, it
        // remains pending and the WFI becomes a nop, and the device never goes
        // in sleep mode. WFE events are latched in a separate pending register
        // so interrupts do not interfere with them
        __WFE();
        
        #if TIMER_DEBUG>0
        wakeup::high();
        #endif//TIMER_DEBUG
         
        SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
        PWR->CR &= ~PWR_CR_LPDS;
        
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
        
        RTC->CRH &=~RTC_CRH_ALRIE;
        #if TIMER_DEBUG>0
        pll::high();
        #endif//TIMER_DEBUG
    }
}

void Rtc::sleep(unsigned long long value)
{
    absoluteSleep(getValue()+value);
}


Rtc::Rtc()
{
    trigger::mode(Mode::OUTPUT);
    #if TIMER_DEBUG >0
    wakeup::mode(Mode::OUTPUT);
    pll::mode(Mode::OUTPUT);
    syncVht::mode(Mode::OUTPUT);
    #endif//TIMER_DEBUG
    FastInterruptDisableLock dLock;
    clockin::mode(Mode::INPUT);
    clockout::mode(Mode::OUTPUT);
    RCC->APB1ENR |= RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_DBP;
    RCC->BDCR |= RCC_BDCR_LSEON     //Enable 32KHz oscillator
               | RCC_BDCR_RTCEN     //Enable RTC
               | RCC_BDCR_RTCSEL_0; //Connect the two together
    while((RCC->BDCR & RCC_BDCR_LSERDY)==0) ; //Wait
    RTC->CRL |= RTC_CRL_CNF;
    RTC->PRLH=(rtcPrescaler>>16) & 0xf;
    RTC->PRLL=rtcPrescaler; //Divide by 2, clock is 16384Hz
    RTC->CNTL=0;
    RTC->CNTH=0;
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

#if TIMER_DEBUG==4
typeTimer VHT::getInfo() const
{
    return info;
}
#endif //TIMER_DEBUG

VHT& VHT::instance()
{
    static VHT timer;
    return timer;
}

unsigned long long VHT::getValue() const
{
    unsigned long long a;
    unsigned short vhtCnt;
    do {
        a=vhtOverflows;
        vhtCnt=TIM4->CNT;
    } while(a!=vhtOverflows); //Ensure no updates in the middle
    return (vhtOverflows|vhtCnt)-vhtSyncPointVht+vhtBase+offset;
}

void VHT::setValue(unsigned long long value)
{
    //We don't actually overwrite the RTC in this case, as the VHT
    //is a bit complicated, so we translate between the two times at
    //the boundary of this class.
    unsigned long long signedNowOld=getValue();
    unsigned long long signedNowNew=value;
    offset=(signedNowNew-signedNowOld);
}

unsigned long long VHT::getExtEventTimestamp() const
{
    return timestampEvent-vhtSyncPointVht+vhtBase+offset;
}

void VHT::absoluteWait(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    vhtWakeupWait=value-vhtBase+vhtSyncPointVht-offset;
    TIM4->SR =~TIM_SR_CC2IF;           //reset interrupt flag channel 2
    TIM4->CCR2=vhtWakeupWait & 0xFFFF;  //set match register channel 2
    TIM4->DIER |= TIM_DIER_CC2IE;       //enable interrupt channel 2
    //Check that wakeup is not in the past.
    if(vhtWakeupWait > 
            ((vhtOverflows+((TIM4->SR & TIM_SR_UIF)?1<<16:0)) | TIM4->CNT))
    {
        vhtInt.wait=false;
        while(!vhtInt.wait)
        {
            vhtWaiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    }
    TIM4->DIER &=~TIM_DIER_CC2IE;
}

bool VHT::absoluteWaitTimeoutOrEvent(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    if(value)
    {
        vhtWakeupWait=value-vhtBase+vhtSyncPointVht-offset;
        TIM4->SR =~(TIM_SR_CC2IF |         //reset interrupt flag channel 2
                     TIM_SR_CC3IF);         //reset interrupt flag channel 3
        TIM4->CCR2=vhtWakeupWait & 0xFFFF;  //set match register channel 2
        TIM4->DIER |= TIM_DIER_CC2IE        //Enable interrupt channel 2
                   |  TIM_DIER_CC3IE;       //Enable interrupt channel 3
        //Check that wakeup is not in the past.
        if(vhtWakeupWait >
            ((vhtOverflows+((TIM4->SR & TIM_SR_UIF)?1<<16:0)) | TIM4->CNT))
        {
            vhtInt.wait=false;
            vhtInt.event=false;
            while(!vhtInt.wait && !vhtInt.event)
            {
                vhtWaiting=Thread::IRQgetCurrentThread();
                Thread::IRQwait();
                {
                    FastInterruptEnableLock eLock(dLock);
                    Thread::yield();
                }
            }
        }
        TIM4->DIER &=~ (TIM_DIER_CC2IE |  TIM_DIER_CC3IE); 
    }
    else
    {
        TIM4->SR =~TIM_SR_CC3IF;           //reset interrupt flag channel 3
        TIM4->DIER |=TIM_DIER_CC3IE;       //Enable interrupt channel 3
        vhtInt.event=0;
        while(!vhtInt.event)
        {
            vhtWaiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
        TIM4->DIER &=~ TIM_DIER_CC3IE;
        return false;
    }
    return vhtInt.wait;
}

void VHT::absoluteTriggerEvent(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    //base case: wakeup is not in this turn 
    vhtWakeupWait=value-vhtBase+vhtSyncPointVht-offset;
    TIM4->CCER &=~TIM_CCER_CC4E;
    TIM4->SR =~TIM_SR_CC4IF;  //reset interrupt flag channel 4
    TIM4->CCR4=vhtWakeupWait & 0xFFFF;  //set match register channel 4
    TIM4->DIER |= TIM_DIER_CC4IE;  
    long long diff = vhtWakeupWait-((vhtOverflows+((TIM4->SR & TIM_SR_UIF)?1<<16:0)) | TIM4->CNT);
    //check if wakeup is in this turn
    if(diff>0 && diff<= 0xFFFFll)
    {
        TIM4->CCER |= TIM_CCER_CC4E;
    }
    else
    {
        // else check if wakeup is in the past
        if(diff<=0)
        {
            TIM4->DIER &=~TIM_DIER_CC4IE;
            TIM4->CCER &=~TIM_CCER_CC4E;
        }
    }
    FastInterruptEnableLock eLock(dLock);  
}

void VHT::absoluteWaitTriggerEvent(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    //base case: wakeup is not in this turn 
    vhtWakeupWait=value-vhtBase+vhtSyncPointVht-offset;
    TIM4->CCER &=~TIM_CCER_CC4E;
    TIM4->SR =~TIM_SR_CC4IF;  //reset interrupt flag channel 4
    TIM4->CCR4=vhtWakeupWait & 0xFFFF;  //set match register channel 4
    TIM4->DIER |= TIM_DIER_CC4IE;  
    vhtInt.trigger=false; 
    long long diff = vhtWakeupWait-((vhtOverflows+((TIM4->SR & TIM_SR_UIF)?1<<16:0)) | TIM4->CNT);
    //check if wakeup is in this turn
    if(diff>0 && diff<= 0xFFFFll)
    {
        TIM4->CCER |= TIM_CCER_CC4E;
    }
    else
    {
        // else check if wakeup is in the past
        if(diff<=0)
        {
            TIM4->DIER &=~TIM_DIER_CC4IE;
            TIM4->CCER &=~TIM_CCER_CC4E;
            vhtInt.trigger = true;
        }
    }
    while(!vhtInt.trigger)
    {
        vhtWaiting=Thread::IRQgetCurrentThread();
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
    TIM4->DIER &=~TIM_DIER_CC1IE;
}

void VHT::absoluteSleep(unsigned long long value)
{
    long long t = value - offset;
    //Unfortunately on the stm32vldiscovery the rtc runs at 16384,
    //while the other timer run at a submultiple of 24MHz, 24MHz in
    //the current setting, and since 24MHz is not a multiple of 16384
    //the conversion is a little complex and requires the use of
    //64bit numbers for intermendiate results. If the main XTAL was
    //8.388608MHz instead of 8MHz a simple shift operation on 32bit
    //numbers would suffice.
    if(t<0) t=0;
    unsigned long long conversion=t;
    conversion*=rtcFreq;
    conversion+=vhtFreq/2; //Round to nearest
    conversion/=vhtFreq;
    rtc.absoluteSleep(conversion);
    synchronizeWithRtc();
    #if TIMER_DEBUG>0
    syncVht::high();
    #endif
}

void VHT::sleep(unsigned long long value)
{
    absoluteSleep(getValue()+value);
    synchronizeWithRtc();
}


void VHT::wait(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    
    unsigned long long a;
    unsigned short vhtCnt;
    do {
        a=vhtOverflows;
        vhtCnt=TIM4->CNT;
    } while(a!=vhtOverflows); //Ensure no updates in the middle
    
    vhtWakeupWait=(vhtOverflows|vhtCnt)+value;
    
    TIM4->SR =~TIM_SR_CC2IF;           //reset interrupt flag channel 2
    TIM4->CCR2=vhtWakeupWait & 0xFFFF;  //set match register channel 2
    TIM4->DIER |= TIM_DIER_CC2IE;       //enable interrupt channel 2
    
    if(vhtWakeupWait > ((vhtOverflows+((TIM4->SR & TIM_SR_UIF)?1<<16:0)) | TIM4->CNT))
    {
        vhtInt.wait=false;
        while(!vhtInt.wait)
        {
            vhtWaiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    }
    TIM4->DIER &=~TIM_DIER_CC2IE;
}

void VHT::synchronizeWithRtc()
{
    {
        FastInterruptDisableLock dLock;    //interrupt disable
        TIM4->CNT=0;
        TIM4->EGR=TIM_EGR_UG;
        
        //These two wait make sure the next code is run exactly after
        //the falling edge of the 16KHz clock
        while(clockin::value()==0) ;
        while(clockin::value()==1) ;
        
        //This delay is extremely important. There appears to be some
        //kind of resynchronization between clock domains happening
        //and reading the RTC registers immediately after the falling
        //edge sometimes returns the old value, and sometimes the new
        //one. This jitter is unacceptable, and the delay fixes it.
        delayUs(18);
        //Get RTC value
        unsigned int a,b;
        a=RTC->CNTL;
        b=RTC->CNTH;
        
        vhtInt.sync=false;
        TIM4->SR =~TIM_SR_CC1IF;     //Disable interrupt flag on channel 1
        TIM4->DIER |= TIM_DIER_CC1IE; //Enable interrupt on channel 1
        
        vhtSyncPointRtc=getRTC64bit(a | b<<16);
        
        while(!vhtInt.sync)
        {
            vhtWaiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
        TIM4->DIER &=~TIM_DIER_CC1IE;
    }
    //Unfortunately on the stm32vldiscovery the rtc runs at 16384,
    //while the other timer run at a submultiple of 24MHz, 1MHz in
    //the current setting, and since 1MHz is not a multiple of 16384
    //the conversion is a little complex and requires the use of
    //64bit numbers for intermendiate results. If the main XTAL was
    //8.388608MHz instead of 8MHz a simple bitmask operation on 32bit
    //numbers would suffice.
    
    unsigned long long conversion=vhtSyncPointRtc;
    conversion*=vhtFreq;
    conversion+=rtcFreq/2; //Round to nearest
    conversion/=rtcFreq;
    vhtBase=conversion;
}

VHT::VHT() : rtc(Rtc::instance()), vhtBase(0), offset(0)
{
    trigger::mode(Mode::ALTERNATE);
    //enable TIM4
    {
        FastInterruptDisableLock dLock;
        RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
       // RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    }
    
    //TIM4 PARTIAL REMAP (CH1/PB4, CH2/PB5, CH3/PB0, CH4/PB1)
    //AFIO->MAPR = AFIO_MAPR_TIM3_REMAP_1;
    
    TIM4->CNT=0;
    TIM4->PSC=vhtPrescaler;  //  24000000/24000000-1; //High frequency timer runs @24MHz
    TIM4->ARR=0xFFFF; //auto reload if counter register go in overflow
    TIM4->CR1= TIM_CR1_URS;
    //setting cc register1 for channel 1 and 2
    TIM4->CCMR1=TIM_CCMR1_CC1S_0  //CC1 connected as input (rtc freq for resync)
              | TIM_CCMR1_IC1F_0; //Sample at 24MHz, resynchronize with 2 samples
    //setting cc register2 for channel 3 and 4
    TIM4->CCMR2=TIM_CCMR2_CC3S_0  //CC3 connected to input 3 (cc2520 SFD)
              | TIM_CCMR2_IC3F_0  //Sample at 24MHz, resynchronize with 2 samples
              | TIM_CCMR2_OC4M_0;
    TIM4->CCER= TIM_CCER_CC1E | TIM_CCER_CC3E; //enable channel
    TIM4->DIER=TIM_DIER_UIE;  //Enable interrupt event @ end of time to set flag
    TIM4->CR1|=TIM_CR1_CEN;
    NVIC_SetPriority(TIM4_IRQn,5); //Low priority
    NVIC_ClearPendingIRQ(TIM4_IRQn);
    NVIC_EnableIRQ(TIM4_IRQn);
    
    synchronizeWithRtc();
}
#else //_BOARD_STM32VLDISCOVERY
#error "rtc.cpp not implemented for the selected board"
#endif //_BOARD_STM32VLDISCOVERY