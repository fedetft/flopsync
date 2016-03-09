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

#define TIMER_DEBUG 2 // 0 no debug; 1 soft debug; 2 pedantic debug; 4 for test;

#if TIMER_DEBUG >0
#include <cstdio>
#include "../board_setup.h"
#endif//TIMER_DEBUG

#ifdef _BOARD_STM32VLDISCOVERY
#define ccoRtc 32768ll
#define ccoVht 24000000ll
#define rtcPrescaler 1    //rtcFreq = ccoRtc/(rtcPrescaler+1) go from 1 to 2^20
#define vhtPrescaler 1-1   //vhtFreq = ccoVht/(vhtPrescaler+1) go from 0 to 2^16 
#define rtcFreq ccoRtc/(rtcPrescaler+1)
#define vhtFreq ccoVht/(vhtPrescaler+1)
#elif defined(_BOARD_WANDSTEM)
#define ccoRtc 32768ll
#define ccoVht 48000000ll
#define rtcPrescaler 0 //rtcFreq = ccoRtc/(rtcPrescaler+1) go from 0 to 15
#define vhtPrescaler 0 //vhtFreq = ccoVht/(vhtPrescaler+1) go from 0 to ? 
#define rtcFreq ccoRtc/(rtcPrescaler+1)
#define vhtFreq ccoVht/(vhtPrescaler+1)
#endif

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

#ifdef _BOARD_STM32VLDISCOVERY
typedef void (*pinirq)();
pinirq IRQreplaceExtiIrq(pinirq newirq);
#endif //_BOARD_STM32VLDISCOVERY

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
     * \return the precise time when the IRQ signal of the event was asserted.
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
     * Puts the thread in wait for the specified absolute time.
     * \param value absolute time in which the thread is waiting.
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     */
    virtual void absoluteWait(unsigned long long value)=0;
    
//    /**
//     * Set the timer interrupt to occur at an absolute value.
//     * When the timer interrupt will occur, the associated GPIO passes 
//     * from a low logic level to a high logic level for few us. 
//     * \param value absolute value when the interrupt will occur, expressed in 
//     * number of tick of the count rate of timer.
//     */
//    virtual void absoluteTrigger(unsigned long long value)=0;
    
    /**
     * Set the timer interrupt to occur at an absolute value and put the 
     * thread in wait of this. 
     * When the timer interrupt will occur, the associated GPIO passes 
     * from a low logic level to a high logic level for few us.
     * \param value absolute value when the interrupt will occur, expressed in 
     * number of tick of the count rate of timer.
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     */
    virtual void absoluteWaitTrigger(unsigned long long value)=0;
    
    /**
     * Put thread in wait for the specified relative time.
     * This function wait for a relative time passed as parameter.
     * @param value relative time to wait, expressed in number of tick of the 
     * count rate of timer.
     */
    virtual void wait(unsigned long long value)=0;
    
    /**
     * Put thread in waiting of timeout or extern event.
     * \param value absolute timeout expressed in number of tick of the 
     * count rate of timer. If zero, the timeout is disabled, and this function 
     * waits indefinitely till the external event will be asserts.
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     */
    virtual bool absoluteWaitTimeoutOrEvent(unsigned long long value)=0;
    
    /**
     * Puts the microcontroller in low power mode (few uA) and wait for the
     * absolute time.
     * \param alue absolute timeout expressed in number of tick of the 
     * count rate of timer.
     * If value of absolute time is in the past no sleep will be set
     * and function return immediately.
     */
    virtual void absoluteSleep(unsigned long long value)=0;
    
    /**
     * Puts the microcontroller in low power mode (few uA) and wait for the
     * relative time.
     */
    virtual void sleep(unsigned long long value)=0;
};

/**
 * Manages the 16384Hz hardware timer that runs also in low power mode
 * 
 * NOTE: in EFM32GG332F1024 the RTC has two compare channels, used in this way:
 * -CMP0 -> used for wait and trigger
 * -CMP1 -> used for VHT resync
 */
class Rtc : public Timer
{
public:
    /**
     * \return an instance to the RTC (singleton)
     */
    static Rtc& instance();
    
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
     * \return the precise time when the IRQ signal of the event was asserted.
     * This member function has two possible implementations: in case no hardware
     * input capture module is used, this function is the same as getValue(), and
     * simply return the current time, otherwise if the underlying timer has an
     * input capture event connected with the radio IRQ signal the time when the
     * IRQ was asserted is returned. Note that you should call this member function
     * <b>as soon as</b> the packet is received, because if the implementation #1
     * is used, this function just returns the current time
     */
    unsigned long long getExtEventTimestamp() const;
    
    /**
     * Puts the thread in wait for the specified absolute time.
     * \param value absolute time in which the thread is waiting.
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     */
    void absoluteWait(unsigned long long value);
    
//    /**
//     * Set the timer interrupt to occur at an absolute value.
//     * When the timer interrupt will occur, the associated GPIO passes 
//     * from a low logic level to a high logic level for few us. 
//     * number of tick of the count rate of timer.
//     */
//    void absoluteTrigger(unsigned long long value);
    
    /**
     * Set the timer interrupt to occur at an absolute value and put the 
     * thread in wait of this. 
     * When the timer interrupt will occur, the associated GPIO passes 
     * from a low logic level to a high logic level for few us. 
     * \param value absolute value when the interrupt will occur, expressed in 
     * number of tick of the count rate of timer.
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     */
    void absoluteWaitTrigger(unsigned long long value);
    
    /**
     * Put thread in wait for the specified relative time.
     * This function wait for a relative time passed as parameter.
     * @param value relative time to wait, expressed in number of tick of the 
     * count rate of timer.
     */
    void wait(unsigned long long value);
    
    /**
     * Put thread in waiting of timeout or extern event.
     * \param value absolute timeout expressed in number of tick of the 
     * count rate of timer. If zero, the timeout is disabled, and this function 
     * waits indefinitely till the external event will be asserts.
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     */
    bool absoluteWaitTimeoutOrEvent(unsigned long long value);
    
    /**
     * Puts the microcontroller in low power mode (few uA) and wait for the
     * absolute time.
     * \param alue absolute timeout expressed in number of tick of the 
     * count rate of timer.
     * If value of absolute time is in the past no sleep will be set
     * and function return immediately.
     */
    void absoluteSleep(unsigned long long value);
    
    /**
     * Puts the microcontroller in low power mode (few uA) and wait for the
     * relative time.
     */
    void sleep(unsigned long long value);
    
private:
    /**
     * Constructor
     */
    Rtc();  
};

// #ifdef _BOARD_STM32VLDISCOVERY

/**
 * Virtual high resolution timer. 
 * This timer uses the RTC and a secondary quartz to create a virtual timer with
 * higher resolution, combining with the advantages of the RTC (active in sleep,
 * low power consumption, good accuracy) and those of a high-resolution timer.
 * The RTC should not be used if a VHT is instantiated because it uses the same 
 * inside.
 * Timer used: TIM4 with 4 channel for I/O capture compare
 * Channel: 2 used as Output compare for implement wait function
 *          4 used as Output compare for start send packet
 *          3 used as Input capture for timestamping SFD
 *          1 used as Input capture for synchronize function (input rtc clock)
 * Note:
 * do not use channel 1 or 2 of TIM3 for output compare in non frozen mode because
 * doesn't work.
 * 
 * EFM32GG332F1024 function mapping, is used TIMER2
 * 
 * CH0 -> timestamp in
 * CH1 -> external trigger & wait
 * CH2 -> 32kHz input 
 */
class VHT : public Timer
{
public:
    /// Unless set differently with setAutoSyncWhitRtcPeriod, this is the period
    ///in ticks of the RTC, at which the VHT resynchronizes
    static const unsigned int defaultAutoSyncPeriod=20;
    
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
     * \return the precise time when the IRQ signal of the event was asserted.
     * This member function has two possible implementations: in case no hardware
     * input capture module is used, this function is the same as getValue(), and
     * simply return the current time, otherwise if the underlying timer has an
     * input capture event connected with the radio IRQ signal the time when the
     * IRQ was asserted is returned. Note that you should call this member function
     * <b>as soon as</b> the packet is received, because if the implementation #1
     * is used, this function just returns the current time
     */
    unsigned long long getExtEventTimestamp() const;
    
    /**
     * Puts the thread in wait for the specified absolute time.
     * \param value absolute time in which the thread is waiting.
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     */
    void absoluteWait(unsigned long long value);
    
//    /**
//     * Set the timer interrupt to occur at an absolute value.
//     * When the timer interrupt will occur, the associated GPIO passes 
//     * from a low logic level to a high logic level for few us.
//     * \param value absolute value when the interrupt will occur, expressed in 
//     * number of tick of the count rate of timer.
//     */
//    void absoluteTrigger(unsigned long long value);
    
    /**
     * Set the timer interrupt to occur at an absolute value and put the 
     * thread in wait of this. 
     * When the timer interrupt will occur, the associated GPIO passes 
     * from a low logic level to a high logic level for few us.
     * \param value absolute value when the interrupt will occur, expressed in 
     * number of tick of the count rate of timer.
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     */
    void absoluteWaitTrigger(unsigned long long value);
    
    /**
     * Put thread in wait for the specified relative time.
     * This function wait for a relative time passed as parameter.
     * @param value relative time to wait, expressed in number of tick of the 
     * count rate of timer.
     */
    void wait(unsigned long long value);
    
    /**
     * Put thread in waiting of timeout or extern event.
     * \param value absolute timeout expressed in number of tick of the 
     * count rate of timer. If zero, the timeout is disabled, and this function 
     * waits indefinitely till the external event will be asserts.
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     */
    bool absoluteWaitTimeoutOrEvent(unsigned long long value);
    
    /**
     * Puts the microcontroller in low power mode (few uA) and wait for the
     * absolute time.
     * \param alue absolute timeout expressed in number of tick of the 
     * count rate of timer.
     * If value of absolute time is in the past no sleep will be set
     * and function return immediately.
     */
    void absoluteSleep(unsigned long long value);
    
    /**
     * Puts the microcontroller in low power mode (few uA) and wait for the
     * relative time.
     */
    void sleep(unsigned long long value);
    
    /**
     * Synchronize the VHT timer with the low frequency RTC
     */
    void syncWithRtc();
    
    /**
     * Allows you to enable periodic synchronization of the VHT with the rtc.
     */
    void enableAutoSyncWhitRtc();
    
    /**
     * Stop the clocks synchronization with the periodic VHT rtc possibly activated.
     */
    void disableAutoSyncWithRtc();
    
    /**
     * 
     * @return true if auto-syncronize with rtc is enabled false otherwise;
     */
    bool isAutoSync();
    
    /**
     * Allows you to set the period of synchronization of the VHT with the rtc.
     * @param period number of tick of rtc, at least 2 or more.
     */
    void setAutoSyncWhitRtcPeriod(unsigned int period);
    
    #if TIMER_DEBUG==4
    typeTimer getInfo() const;
    #endif //TIMER_DEBUG
    
private:
    /**
     * Constructor
     */
    VHT();
    
    void enableAutoSyncHelper(unsigned int period);
    
    Rtc& rtc; //The underlying rtc
    bool autoSync;
};


struct typeVecInt{
    
    unsigned char wait    : 1;
    unsigned char trigger : 1;
    unsigned char event   : 1;
    unsigned char sync    : 1;
    unsigned char flag    : 1;
};

#endif //TIMER_H
