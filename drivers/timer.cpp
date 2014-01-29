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
#include <../miosix/kernel/scheduler/scheduler.h>
#include "timer.h"

using namespace miosix;

#ifdef _BOARD_STM32VLDISCOVERY

#define ccoRtc 32768
#define ccoVht 24000000
#define rtcPrescaler 1    //rtcFreq = ccoRtc/(rtcPrescaler+1) must start from 1 to 2^20
#define vhtPrescaler 0    //vhtFreq = ccoVht/(vhtPrescaler+1) can start from 0 to 2^16 
#define rtcFreq ccoRtc/(rtcPrescaler+1)
#define vhtFreq ccoVht/(vhtPrescaler+1)

static Thread *rtcWaiting=0;        ///< Thread waiting for the RTC interrupt
static Thread *vhtWaiting=0;        ///< Thread waiting on VHT
static Thread *timeoutWaiting=0;    ///< Thread waiting on timeout

static volatile bool vhtIntWait=false;
static volatile bool vhtIntSync=false;
static volatile bool vhtIntEvent=false;
static volatile bool rtcInterrupt=false; ///< The RTC interrupt has fired
static volatile bool eventRecived=false; ///< A packet was received
static void (*eventHandler)(unsigned int)=0; ///< Called when event received
static unsigned long long vhtSyncPointRtc=0;     ///< Rtc time corresponding to vht time
static volatile unsigned long long vhtSyncPointVht=0;     ///< Vht time corresponding to rtc time
static volatile unsigned long long vhtOverflows=0;  //< counter VHT
static volatile unsigned long long timestampEvent=0; ///< input capture timestamp
static unsigned int rtcOverflows=0;        ///< counter RTC overflows
static unsigned int rtcLast=0;             ///< variable for evaluate overflow
static unsigned long long vhtWakeupWait=0;

typedef Gpio<GPIOC_BASE,13> clockout;
typedef Gpio<GPIOB_BASE,1> clockin;

static unsigned long long inline getRTC64bit(unsigned int rtcCounter)
{
    if(rtcCounter<rtcLast) rtcOverflows++;
    rtcLast=rtcCounter;
    unsigned long long result = rtcOverflows;
    result <<= 32;
    result |= rtcCounter;
    return result;
}

static unsigned long long inline getVHT64bit(unsigned short vhtCounter)
{
    return (vhtOverflows << 16) | vhtCounter;
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
    RTC->CRL &= ~RTC_CRL_ALRF;
    EXTI->PR=EXTI_PR_PR17;
    rtcInterrupt=true;
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
    eventRecived=true;
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
    bool wakeup=false;
    //VHT wait output compare channel 1 
    if((TIM3->SR & TIM_SR_CC1IF) && (TIM3->DIER & TIM_DIER_CC1IE))
    {
        TIM3->SR &=~TIM_SR_CC1IF;
        unsigned short timeCapture=TIM3->CCR1;
        unsigned long long ovf=vhtOverflows;
        //IRQ overflow
        if((TIM3->SR & TIM_SR_UIF) && (timeCapture <= TIM3->CNT))
        {
            ovf++;
        }
        //<= is necessary for allow wakeup missed
        if(vhtWakeupWait>>16 <= ovf)    
        {
            vhtIntWait=true;
            wakeup=true;
        }
    }
    
//    //Send packet output compare channel 2 
//    if((TIM3->SR & TIM_SR_CC2IF) && (TIM3->DIER & TIM_DIER_CC2IE))
//    {
//    }
    
    //IRQ input capture channel 3
    if((TIM3->SR & TIM_SR_CC3IF) && (TIM3->DIER & TIM_DIER_CC3IE))
    {
        TIM3->SR &=~TIM_SR_CC3IF;
        vhtIntEvent=true;
        unsigned short timeCapture=TIM3->CCR3;
        VHT::ovh=vhtOverflows;
        timestampEvent=getVHT64bit(timeCapture);
        VHT::counter= TIM3->CNT;
        
        //IRQ overflow
        if((TIM3->SR & TIM_SR_UIF) && (timeCapture <= TIM3->CNT))
            timestampEvent+=0xFFFF;
        wakeup=true;
     }
    
    //vhtSyncvht input capture channel 4
    if((TIM3->SR & TIM_SR_CC4IF) && (TIM3->DIER & TIM_DIER_CC4IE))
    {
        TIM3->SR &=~TIM_SR_CC4IF;
        vhtIntSync = true;
        unsigned short timeCapture=TIM3->CCR4;
        vhtSyncPointVht = getVHT64bit(timeCapture);
        //IRQ overflow
        if((TIM3->SR & TIM_SR_UIF) && (timeCapture <= TIM3->CNT))
            vhtSyncPointVht+=0xFFFF;
        wakeup=true;  
    }
    
    //IRQ overflow
    if(TIM3->SR & TIM_SR_UIF)
    {
        TIM3->SR &=~TIM_SR_UIF;
        vhtOverflows++;
    }
   
    //disalbe overcapture interrupt flag
    TIM3->SR &=~(TIM_SR_CC1OF |TIM_SR_CC2OF | TIM_SR_CC3OF | TIM_SR_CC4OF );
    if(!wakeup || !vhtWaiting) return;
    vhtWaiting->IRQwakeup();
    if(vhtWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    vhtWaiting=0;
}


volatile unsigned short VHT::counter=0;
volatile unsigned long long VHT::ovh=0;

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

void Rtc::setAbsoluteWakeupWait(unsigned long long value)
{
    RTC->CRH |= RTC_CRH_ALRIE;
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRL=value & 0xffff;
    RTC->ALRH=value>>16;
    RTC->CRL &= ~RTC_CRL_CNF;
    rtcInterrupt=false;
    while((RTC->CRL & RTC_CRL_RTOFF)==0) ; //Wait
    if(value <= getValue()) 
        rtcInterrupt = true;
    else
        rtcInterrupt = false;
}

void Rtc::setRelativeWakeup(unsigned int value)
{
    setAbsoluteWakeupWait(getValue()+value);
}

void Rtc::setAbsoluteTimeout(unsigned long long value)
{
    rtcInterrupt=false;
    eventRecived=false;
    if(value!=0)
    {
        setAbsoluteWakeupWait(value);
    }
}
bool Rtc::waitForExtEventOrTimeout()
{
    FastInterruptDisableLock dLock;
    
    while(eventRecived==false && rtcInterrupt==false)
    {
        rtcWaiting=Thread::IRQgetCurrentThread();
        timeoutWaiting = rtcWaiting;
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
    return !eventRecived;
}

void Rtc::wait()
{
    FastInterruptDisableLock dLock;
    while(rtcInterrupt==false)
    {
        rtcWaiting=Thread::IRQgetCurrentThread();
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
}

void Rtc::setAbsoluteWakeupSleep(unsigned long long value)
{
    setAbsoluteWakeupWait(value); //For RTC there's no difference
}

void Rtc::sleep()
{
    while(Console::txComplete()==false) ;

    {
        FastInterruptDisableLock dLock;
        if(rtcInterrupt) return;

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
        vhtCnt=TIM3->CNT;
    } while(a!=vhtOverflows); //Ensure no updates in the middle
    return getVHT64bit(vhtCnt)-vhtSyncPointVht+vhtBase+offset;
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

void VHT::setAbsoluteWakeupWait(unsigned long long value)
{
    vhtWakeupWait=value-vhtBase+vhtSyncPointVht-offset;
    vhtIntWait = false;
    TIM3->SR &=~TIM_SR_CC1IF;           //reset interrupt flag channel 1
    TIM3->CCR1=vhtWakeupWait & 0xFFFF;  //set match register channel 1
    TIM3->DIER |= TIM_DIER_CC1IE;       //enable interrupt channel 1
    //Check that wakeup is not in the past.
    //To be strict. I​should check that the vhtWakeupWait> getValue (),
    //but do not do it to avoid missing wakeup too small.
    //In this mode if the 48 MSB of vhtWakeupWait are equal to vhtOverflow 
    //but the 16 LSB are less than the vht counter, I will wait for more time,
    //at most for one turn of vht counter.
    if(vhtWakeupWait >>16 < vhtOverflows)
        vhtIntWait=true;
}

void VHT::wait()
{
    FastInterruptDisableLock dLock;
    while(!vhtIntWait)
    {
        vhtWaiting=Thread::IRQgetCurrentThread();
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
    TIM3->DIER &=~ TIM_DIER_CC1IE;      //disable interrupt
}

void VHT::setAbsoluteTimeout(unsigned long long value)
{
    vhtIntEvent = false;
    vhtIntWait = false;
    if(value!=0)
    {
        vhtWakeupWait=value-vhtBase+vhtSyncPointVht-offset;
        TIM3->SR &=~(TIM_SR_CC1IF |         //reset interrupt flag channel 1
                     TIM_SR_CC3IF);         //reset interrupt flag channel 3
        TIM3->CCR1=vhtWakeupWait & 0xFFFF;  //set match register channel 1
        TIM3->DIER |= TIM_DIER_CC1IE        //Enable interrupt channel 1
                   |  TIM_DIER_CC3IE;       //Enable interrupt channel 3
        //Check that wakeup is not in the past.
        //To be strict. I​should check that the vhtWakeupWait> getValue (),
        //but do not do it to avoid missing wakeup too small.
        //In this mode if the 48 MSB of vhtWakeupWait are equal to vhtOverflow 
        //but the 16 LSB are less than the vht counter, I will wait for more time,
        //at most for one turn of vht counter.
        if(vhtWakeupWait >>16 < vhtOverflows)
            vhtIntWait=true;
    }else
    {
        TIM3->SR &=~TIM_SR_CC3IF;           //reset interrupt flag channel 3
        TIM3->DIER |=TIM_DIER_CC3IE;        //Enable interrupt channel 3
        
    }
    
}

bool VHT::waitForExtEventOrTimeout()
{
    FastInterruptDisableLock dLock;
    while(!vhtIntEvent && !vhtIntWait)
    {
        vhtWaiting=Thread::IRQgetCurrentThread();
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
    TIM3->DIER &=~(TIM_DIER_CC1IE | TIM_DIER_CC3IE); //disable interrupt
    return !vhtIntEvent;
}

void VHT::setAbsoluteWakeupSleep(unsigned long long value)
{
    long long t = value - offset;
    if(t<=0) return;
    //Unfortunately on the stm32vldiscovery the rtc runs at 16384,
    //while the other timer run at a submultiple of 24MHz, 24MHz in
    //the current setting, and since 24MHz is not a multiple of 16384
    //the conversion is a little complex and requires the use of
    //64bit numbers for intermendiate results. If the main XTAL was
    //8.388608MHz instead of 8MHz a simple shift operation on 32bit
    //numbers would suffice.
    unsigned long long conversion=t;
    conversion*=rtcFreq;
    conversion+=vhtFreq/2; //Round to nearest
    conversion/=vhtFreq;
    rtc.setAbsoluteWakeupSleep(conversion);
}

void VHT::sleep()
{
        rtc.sleep();
        synchronizeWithRtc();
}

void VHT::synchronizeWithRtc()
{
    {
        FastInterruptDisableLock dLock;    //interrupt disable
        TIM3->CNT=0;
        TIM3->EGR=TIM_EGR_UG;
        
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
        
        vhtIntSync=false;
        TIM3->SR &=~TIM_SR_CC4IF;     //Disable interrupt flag on channel 4
        TIM3->DIER |= TIM_DIER_CC4IE; //Enable interrupt on channel 4
        
        vhtSyncPointRtc=getRTC64bit(a | b<<16);
       
        while(!vhtIntSync)
        {
            vhtWaiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
        TIM3->DIER &=~ TIM_DIER_CC4IE; //Disable interrupt on channel 4
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
    //enable TIM3
    {
        FastInterruptDisableLock dLock;
        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    }
    
    TIM3->CNT=0;
    TIM3->PSC=vhtPrescaler;  //  24000000/24000000-1; //High frequency timer runs @24MHz
    TIM3->ARR=0xFFFF; //auto reload if counter register go in overflow
    TIM3->CR1=TIM_CR1_URS; 
    //TIM3->CR1 = TIM_CR1_ARPE; //enable auto reload preload enabled
    
    //setting output compare register1 for channel 1 and 2
    TIM3->CCMR1=TIM_CCMR1_OC2M_0;
    
    //setting input capture register2 for channel 3 and 4
    TIM3->CCMR2=TIM_CCMR2_CC3S_0  //CC3 connected to input 3 (cc2520 SFD)
              | TIM_CCMR2_IC3F_0 //Sample at 24MHz, resynchronize with 2 samples
              | TIM_CCMR2_CC4S_0  //CC4 connected as input (rtc freq for resync)
              | TIM_CCMR2_IC4F_0; //Sample at 24MHz, resynchronize with 2 samples
              
    TIM3->CCER=TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E; //enable channel
    TIM3->DIER=TIM_DIER_UIE;  //Enable interrupt event @ end of time to set flag
    TIM3->CR1=TIM_CR1_CEN;
    NVIC_SetPriority(TIM3_IRQn,10); //Low priority
    NVIC_ClearPendingIRQ(TIM3_IRQn);
    NVIC_EnableIRQ(TIM3_IRQn);
    
    synchronizeWithRtc();
}
#else //_BOARD_STM32VLDISCOVERY
#error "rtc.cpp not implemented for the selected board"
#endif //_BOARD_STM32VLDISCOVERY