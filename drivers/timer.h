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

#ifndef TIMER_H
#define	TIMER_H
#include <miosix.h>
#if TIMER_DEBUG >0
#include <cstdio> 
#endif//TIMER_DEBUG

#define TIMER_DEBUG 1 // 0 no debug; 1 soft debug; 2 pedantic debug; 4 for test;

#define ccoRtc 32768ll
#define ccoVht 24000000ll
#define rtcPrescaler 1    //rtcFreq = ccoRtc/(rtcPrescaler+1) go from 1 to 2^20
#define vhtPrescaler 1-1   //vhtFreq = ccoVht/(vhtPrescaler+1) go from 0 to 2^16 
#define rtcFreq ccoRtc/(rtcPrescaler+1)
#define vhtFreq ccoVht/(vhtPrescaler+1)

#if TIMER_DEBUG==4
struct typeTimer
{
    volatile unsigned long long ovf;
    volatile unsigned short cnt;
    volatile unsigned short sr;
    volatile unsigned long long ts;
    volatile unsigned short cntFirstUIF;
    volatile unsigned short srFirstUIF;
};
#endif //TIMER_DEUB

class Timer
{
public:
    /**
     * \return the timer counter value
     */
    virtual unsigned long long getValue() const=0;

    /**
     * Set the RTC counter value
     * \param value new RTC value
     */
    virtual void setValue(unsigned long long value)=0;
    
    /**
     * \return the precise time when the IRQ signal of the nRF24L01 was asserted.
     * This member function has two possible implementations: in case no hardware
     * input capture module is used, this function is the same as getValue(), and
     * simply return the current time, otherwise if the underlying timer has an
     * input capture event connected with the radio IRQ signal the time when the
     * IRQ was asserted is returned. Note that you should call this member function
     * <b>as soon as</b> the packet is received, because if the implementation #1
     * is used, this function just returns the current time
     */
    virtual unsigned long long getExtEventTimestamp() const=0;
    
    /**
     * Set the timer interrupt to occur at an absolute value.
     * \param value absolute value when the interrupt will occur.
     * If value of wakeup is in the past no interrupt will be set
     * and wait function will do nothing.
     */
    virtual void setAbsoluteWakeupWait(unsigned long long value)=0;
    
      /**
     * Set the timer interrupt to occur at an absolute value.
     * when the timer interrupt will occur, the associated GPIO passes 
     * from a low logic level to a high logic level.
     * If wait() is call before this, we will have the same behavior of the 
     * setAboluteWakeupWait whit more hardware event.
     * \param value absolute value when the interrupt will occur.
     * If value of wakeup is in the past no interrupt will be set
     * and wait function will do nothing.
     */
    virtual void setAbsoluteTriggerEvent(unsigned long long value)=0;
    
    /**
     * Wait for the interrupt.
     * You must have called setAbsoluteWakeupWait() before this 
     * or setAbsoluteTriggerEvent().
     * \return true if timeout occurs false otherwise
     */
    virtual bool wait()=0;
    
    /**
     * Wait for the interrupt.
     * This function wait for a relative time passed as parameter.
     * @param value relative time to wait.
     */
    virtual void wait(unsigned long long value)=0;
    
    /**
     * Set the timer interrupt to occur at an absolute timeout if no external 
     * event arrived before.
     * \param value absolute value when the interrupt will occur. If zero, 
     * the timeout is disabled, and this function waits indefinitely till the 
     * external event will be asserts.
     * If value of wakeup is in the past no interrupt will be set.
     */
    virtual void setAbsoluteTimeoutForEvent(unsigned long long value)=0;
    
    /**
     * Set the timer timer interrupt to occur at an absolute value
     * \param value absolute value when the interrupt will occur.
     * If value of wakeup is in the past no interrupt will be set
     * and sleep function will do nothing.
     */
    virtual void setAbsoluteWakeupSleep(unsigned long long value)=0;
    
    /**
     * Puts the microcontroller in low power mode (few uA) and wait for the
     * interrupt. You must have called setAbsoluteWakeupSleep() before this
     */
    virtual void sleep()=0;
};

/**
 * Manages the 16384Hz hardware timer that runs also in low power mode
 */
class Rtc : public Timer
{
public:
    /**
     * \return an instance to the RTC (singleton)
     */
    static Rtc& instance();
    
    /**
     * \return the RTC counter value
     */
    unsigned long long getValue() const;

    /**
     * Set the RTC counter value
     * \param value new RTC value
     */
    void setValue(unsigned long long value);
    
    /**
     * No input capture used, this is the same as getValue()
     */
    unsigned long long getExtEventTimestamp() const;
    
    /**
     * Set the timer interrupt to occur at an absolute value.
     * \param value absolute value when the interrupt will occur.
     * If value of wakeup is in the past no interrupt will be set
     * and wait function will do nothing.
     */
    void setAbsoluteWakeupWait(unsigned long long value);
    
    /**
     * Wait for the interrupt.
     * This function wait for a relative time passed as parameter.
     * @param value relative time to wait.
     */
    void wait(unsigned long long value);
    
      /**
     * Set the timer interrupt to occur at an absolute value.
     * when the timer interrupt will occur, the associated GPIO passes 
     * from a low logic level to a high logic level.
     * If wait() is call before this, we will have the same behavior of the 
     * setAboluteWakeupWait whit more hardware event.
     * \param value absolute value when the interrupt will occur.
     * If value of wakeup is in the past no interrupt will be set
     * and wait function will do nothing.
     */
    void setAbsoluteTriggerEvent(unsigned long long value);
    
    /**
     * Wait for the interrupt.
     * You must have called setAbsoluteWakeupWait() before this 
     * or setAbsoluteTriggerEvent().
     * \return true if timeout occurs false otherwise
     */
    bool wait();
    
     /**
     * Set the timer interrupt to occur at an absolute timeout if no external 
     * event arrived before.
     * \param value absolute value when the interrupt will occur. If zero, 
     * the timeout is disabled, and this function waits indefinitely till the 
     * external event will be asserts.
     * If value of wakeup is in the past no interrupt will be set.
     */
    void setAbsoluteTimeoutForEvent(unsigned long long value);
    
    /**
     * Set the timer timer interrupt to occur at an absolute value
     * \param value absolute value when the interrupt will occur.
     * If value of wakeup is in the past no interrupt will be set
     * and sleep function will do nothing.
     */
    void setAbsoluteWakeupSleep(unsigned long long value);
    
    /**
     * Puts the microcontroller in low power mode (few uA) and wait for the
     * RTC interrupt
     */
    void sleep();
    
private:
    /**
     * Constructor
     */
    Rtc();  
};

/**
 * Virtual high resolution timer
 * Timer used: TIM3 with 4 channel for I/O capture compare
 * Channel: 2 used as Output compare for implement wait function
 *          4 used as Output compare for start send packet
 *          3 used as Input capture for timestamping SFD
 *          1 used as Input capture for synchronize function (input rtc clock)
 * Note:
 * do not use channel 1 or 2 for output compare in non frozen mode because
 * doesn't work.
 */
class VHT : public Timer
{
public:
    /**
     * \return an instance to the VHT (singleton)
     */
    static VHT& instance();
    
    /**
     * \return the timer counter value
     */
    unsigned long long getValue() const;
    
    /**
     * Set the RTC counter value
     * \param value new RTC value
     */
    void setValue(unsigned long long value);

    /**
     * \return the precise time when the IRQ signal of the nRF24L01 was asserted.
     * This implementation uses an input capture hardware module for additional
     * precision
     */
    unsigned long long getExtEventTimestamp() const;
    
    /**
     * Set the timer interrupt to occur at an absolute value.
     * \param value absolute value when the interrupt will occur.
     * If value of wakeup is in the past no interrupt will be set
     * and wait function will do nothing.
     */
    void setAbsoluteWakeupWait(unsigned long long value);
    
    /**
     * Set the timer interrupt to occur at an absolute value.
     * when the timer interrupt will occur, the associated GPIO passes 
     * from a low logic level to a high logic level.
     * If wait() is call before this, we will have the same behavior of the 
     * setAboluteWakeupWait whit more hardware event.
     * \param value absolute value when the interrupt will occur.
     * If value of wakeup is in the past no interrupt will be set
     * and wait function will do nothing.
     */
    virtual void setAbsoluteTriggerEvent(unsigned long long value);
    
    /**
     * Wait for the interrupt.
     * You must have called setAbsoluteWakeupWait() before this 
     * or setAbsoluteTriggerEvent().
     * \return true if timeout occurs false otherwise
     */
    bool wait();
    
    /**
     * Wait for the interrupt.
     * This function wait for a relative time passed as parameter.
     * @param value relative time to wait.
     */
    void wait(unsigned long long value);
    
    /**
     * Set the timer interrupt to occur at an absolute timeout if no external 
     * event arrived before.
     * \param value absolute value when the interrupt will occur. If zero, 
     * the timeout is disabled, and this function waits indefinitely till the 
     * external event will be asserts.
     * If value of wakeup is in the past no interrupt will be set.
     */
    void setAbsoluteTimeoutForEvent(unsigned long long value);
    
    /**
     * Set the timer timer interrupt to occur at an absolute value
     * \param value absolute value when the interrupt will occur.
     * If value of wakeup is in the past no interrupt will be set
     * and sleep function will do nothing.
     */
    void setAbsoluteWakeupSleep(unsigned long long value);
    
    /**
     * Puts the microcontroller in low power mode (few uA) and wait for the
     * interrupt. You must have called setAbsoluteWakeupSleep() before this
     */
    void sleep();
    
    /**
     * Synchronize the VHT timer with the low frequency RTC
     */
    void synchronizeWithRtc();
    
    #if TIMER_DEBUG==4
    typeTimer getInfo()const;
    #endif //TIMER_DEBUG
    
    
    
    // Ratio between timers 24MHz / 16384Hz. Note that the division is
    // not exact, and this introduces a clock skew by itself, even if
    // the RTC was perfect. So don't use it to make precise conversions
    // from VHT to RTC
    static const unsigned int scaleFactor=1464;
    
private:
    /**
     * Constructor
     */
    VHT();
    
    Rtc& rtc; //The underlying rtc
    unsigned long long vhtBase;
    long long offset; //Only for setting the RTC value
};

/**
 * \param handler a function pointer that will be called when an event is
 * received. Its unsigned int parameter is the timestamp of the event.
 * If NULL is passed, the event handling is disabled. The event handler
 * will be called from within an interrupt or with interrupts disabled,
 * so it should not allocate memory, print, or whatever is forbidden inside
 * an IRQ.
 */
void setEventHandler(void (*handler)(unsigned int));

#endif //TIMER_H
