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

void Rtc::setAbsoluteWakeup(unsigned int value)
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
    setAbsoluteWakeup(getValue()+value);
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

void Rtc::sleepAndWait()
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

#else //_BOARD_STM32VLDISCOVERY
#error "rtc.cpp not implemented for the selected board"
#endif //_BOARD_STM32VLDISCOVERY
