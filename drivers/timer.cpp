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

#define delay_oc 5 //the OCREF go hight 5 tick after real time

static Thread *rtcWaiting=0;        ///< Thread waiting for the RTC interrupt
static Thread *vhtWaiting=0;        ///< Thread waiting on VHT

static volatile typeVecInt rtcInt;
static unsigned int  syncVhtRtcPeriod=20;
static volatile typeVecInt vhtInt;
static volatile unsigned long long vhtBase=0;
static volatile unsigned long long vhtOffset=0;
static volatile bool rtcTriggerEnable=false;
//static void (*eventHandler)(unsigned int)=0; ///< Called when event received
static unsigned long long vhtSyncPointRtc=0;     ///< Rtc time corresponding to vht time
static volatile unsigned long long vhtSyncPointVht=0;     ///< Vht time corresponding to rtc time
static volatile unsigned long long vhtOverflows=0;  //< counter VHT overflows
static volatile unsigned long long timestampEvent=0; ///< input capture timestamp
static unsigned int rtcOverflows=0;        ///< counter RTC overflows
static unsigned int rtcLast=0;             ///< variable for evaluate overflow
static unsigned long long vhtWakeupWait=0;

typedef Gpio<GPIOB_BASE,9> trigger;
typedef Gpio<GPIOC_BASE,13> resyncVHTout;
typedef Gpio<GPIOB_BASE,6> resyncVHTin;

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
    rtcInt.wait=true;
    if(rtcTriggerEnable)
    {
        trigger::high();
        rtcTriggerEnable=false;
        rtcInt.trigger=true;
        RTC->CRL &=~RTC_CRH_ALRIE;
        trigger::low();
    }
    if(!rtcWaiting) return;
    rtcWaiting->IRQwakeup();
	if(rtcWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		Scheduler::IRQfindNextThread();
    rtcWaiting=0;
}

//Commented because EXTI9_5_IRQHandler() is implemented also in the cc2520.
//I can not implement the same Handler in two classes.
/**
 * EXTI9_5 is connected to PB8, this for SFD, FRM_DONE interrupt
 */
//void __attribute__((naked)) EXTI9_5_IRQHandler()
//{
//	saveContext();
//	asm volatile("bl _Z19eventIrqhandlerImplv");
//	restoreContext();
//}

/**
 * Interrupt SFD, FRM_DONE actual implementation
 */
//void __attribute__((used)) eventIrqhandlerImpl()
//{
//     //Get a timestamp of the event
//    unsigned int a,b;
//    do {
//        a=RTC->CNTL;
//        b=RTC->CNTH;
//    } while(a!=RTC->CNTL); //Ensure no updates in the middle
//    timestampEvent=getRTC64bit(a|b<<16);
//    
//    EXTI->PR=EXTI_PR_PR8;
//    EXTI->IMR &= ~EXTI_IMR_MR8;
//    EXTI->RTSR &= ~EXTI_RTSR_TR8;
//    rtcInt.event=true;
//    if(!rtcWaiting) return;
//    rtcWaiting->IRQwakeup();
//	  if(rtcWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
//		Scheduler::IRQfindNextThread();
//    rtcWaiting=0;
//}


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
    //vhtSyncRtc input capture channel 1
    if((TIM4->SR & TIM_SR_CC1IF) && (TIM4->DIER & TIM_DIER_CC1IE))
    { 
        if(vhtInt.flag) 
        {
            vhtInt.sync=true;
            vhtInt.flag=false;
        }
        unsigned long long old_vhtBase = vhtBase;
        unsigned long long old_vhtSyncPointVht=vhtSyncPointVht;
        unsigned long long old_vhtWakeupWait=vhtWakeupWait;
        
        TIM4->SR =~TIM_SR_CC1IF;
        
        vhtSyncPointVht=vhtOverflows|TIM4->CCR1;
        //check if overflow interrupt happened before IC1 interrupt
        if((TIM4->SR & TIM_SR_UIF) && ((vhtSyncPointVht & 0xFFFFll) <= TIM4->CNT))
            vhtSyncPointVht+=1<<16;
        
        //Unfortunately on the stm32vldiscovery the rtc runs at 16384,
        //while the other timer run at a submultiple of 24MHz, 1MHz in
        //the current setting, and since 1MHz is not a multiple of 16384
        //the conversion is a little complex and requires the use of
        //64bit numbers for intermendiate results. If the main XTAL was
        //8.388608MHz instead of 8MHz a simple bitmask operation on 32bit
        //numbers would suffice.
        {
            unsigned long long conversion=vhtSyncPointRtc;
            conversion*=vhtFreq;
            conversion+=rtcFreq/2; //Round to nearest
            conversion/=rtcFreq;
            vhtBase=conversion;
        }
        
        //Update output compare channel if enabled
        if((TIM4->DIER & TIM_DIER_CC2IE) || (TIM4->DIER & TIM_DIER_CC4IE))
        {
            vhtWakeupWait+=old_vhtBase-old_vhtSyncPointVht-vhtBase+vhtSyncPointVht;
            unsigned long long time = (vhtOverflows+((TIM4->SR & TIM_SR_UIF)?1<<16:0)) | TIM4->CNT;
            long long diff = vhtWakeupWait-time;
            long long old_diff = old_vhtWakeupWait-time;
            
            //Update only if the event are at least 100 tick in the future.
            //100 is calibrate for avoid miss event (it is assumed that the following
            //instructions which set the registers of the output compare employing 
            //less than 100 clock cycles)
            if(old_diff>100 && diff>100)
            {
                if(TIM4->DIER & TIM_DIER_CC4IE)
                {
                    TIM4->CCR4=vhtWakeupWait; 
                    //check if wakeup is in this turn
                    if(diff>0 && diff<= 0xFFFFll)
                    {
                        TIM4->CCMR2 |=TIM_CCMR2_OC4M_0; //active level on match
                    }
                }
                TIM4->CCR2=vhtWakeupWait; //also updates if not enabled
            }
            else
            {
                vhtWakeupWait = old_vhtWakeupWait;
            }
        }
        
        //set next resync vht
        vhtSyncPointRtc+=syncVhtRtcPeriod;
        RTC->CRL |= RTC_CRL_CNF; //configuration mode
        RTC->ALRL=vhtSyncPointRtc;
        RTC->ALRH=vhtSyncPointRtc >>16;
        RTC->CRL &= ~RTC_CRL_CNF;
        while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
        #if TIMER_DEBUG>0
        probe_sync_vht::high();
        probe_sync_vht::low();
        #endif
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
        timestampEvent=timestampEvent-vhtSyncPointVht+vhtBase+vhtOffset;
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
            vhtInt.trigger=true;
            TIM4->DIER &=~TIM_DIER_CC4IE;
            TIM4->CCMR2 &=~TIM_CCMR2_OC4M ;  //frozen mode
            TIM4->CCMR2 |=TIM_CCMR2_OC4M_2 ; //force inactive level
            TIM4->CCMR2 &=~TIM_CCMR2_OC4M ;  //frozen mode
        }
        else 
        {
            if(remainingSlot==1)
            {
                TIM4->CCMR2 |=TIM_CCMR2_OC4M_0; //active level on match 
            }
        }
        
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
    RTC->CNTL=value;
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
    RTC->ALRL=value;
    RTC->ALRH=value>>16;
    RTC->CRL &= ~RTC_CRL_CNF;
    vhtSyncPointRtc=value;
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
    rtcInt.wait=false;
}

bool Rtc::absoluteWaitTimeoutOrEvent(unsigned long long value)
{
    bool result=false;
    if(value!=0)
    {
        FastInterruptDisableLock dLock;
        RTC->CRL |= RTC_CRL_CNF;
        RTC->CRL =~RTC_CRL_ALRF;
        RTC->CRH |= RTC_CRH_ALRIE;
        RTC->ALRL=value;
        RTC->ALRH=value>>16;
        RTC->CRL &= ~RTC_CRL_CNF;
        vhtSyncPointRtc=value;
        rtcInt.wait=false;
        rtcInt.event=false;
        while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
        EXTI->IMR |= EXTI_IMR_MR8;
        EXTI->RTSR |= EXTI_RTSR_TR8;
        EXTI->PR=EXTI_PR_PR8; //Clear eventual pending IRQ 
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
            if(rtcInt.wait && rtcInt.event)
            {
                (timestampEvent<=value)? result=false:result=true;
            }
            else
            {
                result = rtcInt.wait;
            }
            result=rtcInt.wait;
            rtcInt.wait=false;
            rtcInt.event=false;
        }
    }
    else
    {
        FastInterruptDisableLock dLock;
        EXTI->IMR |= EXTI_IMR_MR8;
        EXTI->RTSR |= EXTI_RTSR_TR8;
        EXTI->PR=EXTI_PR_PR8; //Clear eventual pending IRQ 
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
        rtcInt.event=false;
        return true;
    }
    return result;
}

void Rtc::absoluteTrigger(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    trigger::low();
    RTC->CRL =~RTC_CRL_ALRF;
    RTC->CRH |= RTC_CRH_ALRIE;
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRL=value;
    RTC->ALRH=value>>16;
    RTC->CRL &= ~RTC_CRL_CNF;
    vhtSyncPointRtc=value;
    rtcTriggerEnable=true;
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
    if(value <= getValue()) 
        rtcTriggerEnable=false;
    FastInterruptEnableLock eLock(dLock);
}

void Rtc::absoluteWaitTrigger(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    trigger::low();
    RTC->CRL =~RTC_CRL_ALRF;
    RTC->CRH |= RTC_CRH_ALRIE;
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRL=value;
    RTC->ALRH=value>>16;
    RTC->CRL &= ~RTC_CRL_CNF;
    vhtSyncPointRtc=value;
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
    rtcInt.trigger=false;
}

void Rtc::wait(unsigned long long value)
{
    absoluteWait(getValue()+value);
}

void Rtc::absoluteSleep(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    
    RTC->CRL =~RTC_CRL_ALRF;
    RTC->CRH |= RTC_CRH_ALRIE;
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRL=value;
    RTC->ALRH=value>>16;
    RTC->CRL &= ~RTC_CRL_CNF;
    rtcInt.wait=false;
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
    EXTI->PR=EXTI_PR_PR17; //Clear eventual pending IRQ
    #if TIMER_DEBUG >0
    assert(getValue()<value); 
    #endif //TIMER_DEBUG
    if(getValue()>value) return;
    
    while(Console::txComplete()==false) ;
    {
        if(rtcInt.wait) return;
        EXTI->EMR |= EXTI_EMR_MR17; //enable event for wakeup
        
        PWR->CR |= PWR_CR_LPDS;
        SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
        
        // Using WFE instead of WFI because if while we are with interrupts
        // disabled an interrupt (such as the tick interrupt) occurs, it
        // remains pending and the WFI becomes a nop, and the device never goes
        // in sleep mode. WFE events are latched in a separate pending register
        // so interrupts do not interfere with them       
        __WFE();
        EXTI->EMR &=~ EXTI_EMR_MR17; //disable event for wakeup
        #if TIMER_DEBUG>0
        probe_wakeup::high();
        probe_wakeup::low();
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
        probe_pll_boot::high();
        probe_pll_boot::low();
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
    FastInterruptDisableLock dLock;
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
    EXTI->RTSR |= EXTI_RTSR_TR17;
    NVIC_ClearPendingIRQ(RTC_IRQn);
    NVIC_SetPriority(RTC_IRQn,10); //Low priority
	NVIC_EnableIRQ(RTC_IRQn);
    
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;         //enable AFIO reg
    AFIO->EXTICR[2] |= AFIO_EXTICR3_EXTI8_PB;   //enable interrupt on PB8
    NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
    NVIC_SetPriority(EXTI9_5_IRQn,10); //low priority
    NVIC_EnableIRQ(EXTI9_5_IRQn);
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
    return (vhtOverflows|vhtCnt)-vhtSyncPointVht+vhtBase+vhtOffset;
}

void VHT::setValue(unsigned long long value)
{
    //We don't actually overwrite the RTC in this case, as the VHT
    //is a bit complicated, so we translate between the two times at
    //the boundary of this class.
    unsigned long long signedNowOld=getValue();
    unsigned long long signedNowNew=value;
    vhtOffset=(signedNowNew-signedNowOld);
}
    
unsigned long long VHT::getExtEventTimestamp() const
{
    return timestampEvent;
}

void VHT::absoluteWait(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    vhtWakeupWait=value-vhtBase+vhtSyncPointVht-vhtOffset;
    TIM4->SR =~TIM_SR_CC2IF;           //reset interrupt flag channel 2
    TIM4->CCR2=vhtWakeupWait;  //set match register channel 2
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
    vhtInt.wait=false;
}

bool VHT::absoluteWaitTimeoutOrEvent(unsigned long long value)
{
    bool result=false;
    FastInterruptDisableLock dLock;
    if(value!=0)
    {
        vhtWakeupWait=value-vhtBase+vhtSyncPointVht-vhtOffset;
        TIM4->SR =~(TIM_SR_CC2IF |         //reset interrupt flag channel 2
                     TIM_SR_CC3IF);         //reset interrupt flag channel 3
        TIM4->CCR2=vhtWakeupWait;  //set match register channel 2
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
        if(vhtInt.wait && vhtInt.event)
        {
            (timestampEvent<=value)? result=false:result=true;
        }
        else
        {
            result = vhtInt.wait;
        }
        vhtInt.wait=false;
        vhtInt.event=false;
    }
    else
    {
        TIM4->SR =~TIM_SR_CC3IF;           //reset interrupt flag channel 3
        TIM4->DIER |=TIM_DIER_CC3IE;       //Enable interrupt channel 3
        vhtInt.event=false;
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
        vhtInt.event=false;
        return false;
    }
    return result;
}

void VHT::absoluteTrigger(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    //base case: wakeup is not in this turn 
    vhtWakeupWait=value-vhtBase+vhtSyncPointVht-vhtOffset-delay_oc;
    TIM4->SR =~TIM_SR_CC4IF;  //reset interrupt flag channel 4
    TIM4->CCR4=vhtWakeupWait;  //set match register channel 4
    TIM4->DIER |= TIM_DIER_CC4IE;  
    vhtInt.trigger=false; 
    long long diff = vhtWakeupWait-((vhtOverflows+((TIM4->SR & TIM_SR_UIF)?1<<16:0)) | TIM4->CNT);
    //check if wakeup is in this turn
    if(diff>0 && diff<= 0xFFFFll)
    {
        TIM4->CCMR2 |=TIM_CCMR2_OC4M_0; //active level on match
    }
    else
    {
        // else check if wakeup is in the past
        if(diff<=0)
        {
            TIM4->DIER &=~TIM_DIER_CC4IE;
            vhtInt.trigger = true;
            TIM4->CCMR2 &=~TIM_CCMR2_OC4M; //frozen mode
        }
    } 
}

void VHT::absoluteWaitTrigger(unsigned long long value)
{
    FastInterruptDisableLock dLock;
    //base case: wakeup is not in this turn 
    vhtWakeupWait=value-vhtBase+vhtSyncPointVht-vhtOffset-delay_oc;
    TIM4->SR =~TIM_SR_CC4IF;            //reset interrupt flag channel 4
    TIM4->CCR4=vhtWakeupWait;  //set match register channel 4
    TIM4->DIER |= TIM_DIER_CC4IE;  
    vhtInt.trigger=false; 
    long long diff = vhtWakeupWait-((vhtOverflows+((TIM4->SR & TIM_SR_UIF)?1<<16:0)) | TIM4->CNT);
    //check if wakeup is in this turn
    if(diff>0 && diff<= 0xFFFFll)
    {
        TIM4->CCMR2 |=TIM_CCMR2_OC4M_0; //active level on match
    }
    else
    {
        // else check if wakeup is in the past
        if(diff<=0)
        {
            TIM4->DIER &=~TIM_DIER_CC4IE;
            vhtInt.trigger = true;
            TIM4->CCMR2 &=~TIM_CCMR2_OC4M; //frozen mode
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
    TIM4->DIER &=~ TIM_DIER_CC4IE;
    vhtInt.trigger=false;
}

void VHT::absoluteSleep(unsigned long long value)
{
    long long t = value - vhtOffset;
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
    syncWithRtc();   
}

void VHT::sleep(unsigned long long value)
{
    absoluteSleep(getValue()+value);
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
    TIM4->CCR2=vhtWakeupWait;  //set match register channel 2
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
    vhtInt.wait=false;
}

void VHT::syncWithRtc()
{
    FastInterruptDisableLock dLock;
    //at least 2 tick is necessary for resynchronize because rtc alarm 
    //enable tamper in the previous cycle of match between counter register and
    //alarm register
    TIM4->SR =~TIM_SR_CC1IF; 
    TIM4->DIER|=TIM_DIER_CC1IE; 
    vhtSyncPointRtc = rtc.getValue()+2;
    RTC->CRL |= RTC_CRL_CNF; //configuration mode
    RTC->ALRL=vhtSyncPointRtc;
    RTC->ALRH=vhtSyncPointRtc>>16;
    RTC->CRL &= ~RTC_CRL_CNF;
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
    vhtInt.sync=false;
    vhtInt.flag=true;
    while(!vhtInt.sync)
    {
        vhtWaiting=Thread::IRQgetCurrentThread();
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
    if(!autoSync)  TIM4->DIER&=~TIM_DIER_CC1IE; 
    vhtInt.sync=false;
    #if TIMER_DEBUG>0
    probe_sync_vht::high();
    probe_sync_vht::low();
    #endif
}

void VHT::enableAutoSyncWhitRtc(unsigned int period)
{
    FastInterruptDisableLock dLock;
    if(period == syncVhtRtcPeriod && autoSync) return;
    autoSync=true;
    if (period<2) syncVhtRtcPeriod = 2;
    TIM4->SR =~TIM_SR_CC1IF; 
    TIM4->DIER|=TIM_DIER_CC1IE; 
    vhtSyncPointRtc = rtc.getValue()+syncVhtRtcPeriod;
    RTC->CRL |= RTC_CRL_CNF; //configuration mode
    RTC->ALRL=vhtSyncPointRtc;
    RTC->ALRH=vhtSyncPointRtc>>16;
    RTC->CRL &= ~RTC_CRL_CNF;
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
}

void VHT::disableAutoSyncWithRtc()
{
    autoSync=false;
    TIM4->SR =~TIM_SR_CC1IF; 
    TIM4->DIER&=~TIM_DIER_CC1IE;
}

bool VHT::isAutoSync()
{
    return autoSync;
}



VHT::VHT() : rtc(Rtc::instance()), autoSync(true)
{
    trigger::mode(Mode::ALTERNATE);
    resyncVHTout::mode(Mode::OUTPUT);
    resyncVHTin::mode(Mode::INPUT);
    BKP->RTCCR=BKP_RTCCR_ASOE; //Enable RTC alarm out
    //enable TIM4
    {
        FastInterruptDisableLock dLock;
        RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    }
    TIM4->CNT=0;
    TIM4->PSC=vhtPrescaler;  //  24000000/24000000-1; //High frequency timer runs @24MHz
    TIM4->ARR=0xFFFF; //auto reload if counter register go in overflow
    TIM4->CR1= TIM_CR1_URS;
    //setting cc register1 for channel 1 and 2
    TIM4->CCMR1=TIM_CCMR1_CC1S_0  //CC1 connected as input (rtc freq for resync)
              | TIM_CCMR1_IC1F_0; //Sample at 24MHz, resynchronize with 2 samples
    //setting cc register2 for channel 3 and 4
    TIM4->CCMR2=TIM_CCMR2_CC3S_0  //CC3 connected to input 3 (cc2520 SFD)
              | TIM_CCMR2_IC3F_0;  //Sample at 24MHz, resynchronize with 2 samples
    TIM4->CCER= TIM_CCER_CC1E|TIM_CCER_CC3E|TIM_CCER_CC4E ;  //enable channel
    TIM4->DIER=TIM_DIER_UIE;  //Enable interrupt event @ end of time to set flag
    TIM4->CR1|=TIM_CR1_CEN;
    NVIC_SetPriority(TIM4_IRQn,5); //Low priority
    NVIC_ClearPendingIRQ(TIM4_IRQn);
    NVIC_EnableIRQ(TIM4_IRQn);
    
    syncWithRtc();
    
}
#else //_BOARD_STM32VLDISCOVERY
#error "rtc.cpp not implemented for the selected board"
#endif //_BOARD_STM32VLDISCOVERY