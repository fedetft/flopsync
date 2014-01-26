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
     * Set the timer interrupt to occur at an absolute value
     * \param value absolute value when the interrupt will occur
     */
    virtual void setAbsoluteWakeupWait(unsigned long long value)=0;
    
    /**
     * Return when either an external event arrived or the wait value setted by
     * setAbsoluteWakeupWait expired.
     * You must have called setAbsoluteWakeupWait() before this.
     * \return true if timeout occurred, false if packet arrived
     */
    virtual bool waitForExtEventOrTimeout()=0;
    
    /**
     * Wait for the interrupt.
     *  You must have called setAbsoluteWakeupWait() before this.
     */
    virtual void wait()=0;
    
    /**
     * Set the timer timer interrupt to occur at an absolute value
     * \param value absolute value when the interrupt will occur
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
     * Set the RTC timer interrupt to occur at an absolute value
     * \param value absolute value when the interrupt will occur
     */
    void setAbsoluteWakeupWait(unsigned long long value);
    
    /**
     * Set the RTC timer interrupt to occur at a relative value.
     * Note: the stm32vldiscovery development board has been chosen because it
     * has an absolute RTC timer. Given that, it easy to implement the relative
     * timer model on top of the absolute one. The opposite is instead not
     * possible.
     * \param value relative value when the interrupt will occur
     */
    void setRelativeWakeup(unsigned int value);
    
    /**
     * Return when either an external event arrived or the wait value setted by
     * setAbsoluteWakeupWait expired.
     * You must have called setAbsoluteWakeupWait() before this.
     * \return true if timeout occurred, false if packet arrived
     */
    bool waitForExtEventOrTimeout();
    
    /**
     * Wait for the interrupt
     */
    void wait();
    
    /**
     * Set the timer interrupt to occur at an absolute value
     * \param value absolute value when the interrupt will occur
     * value must be less than 0xFFFFFFFF + getValue()
     * \param value absolute value when the interrupt will occur
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
 * Channel: 1 used as Output compare for implement wait function
 *          2 used as Output compare for start send packet
 *          3 used as Input capture for timestamping SFD
 *          4 used as Input capture for synchronize function (input rtc clock)
 * 
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
     * Set the timer interrupt to occur at an absolute value
     * \param value absolute value when the interrupt will occur
     */
    void setAbsoluteWakeupWait(unsigned long long value);
    
    /**
     * Return when either an external event arrived or the wait value setted by
     * setAbsoluteWakeupWait expired.
     * You must have called setAbsoluteWakeupWait() before this.
     * \return true if timeout occurred, false if packet arrived
     */
    bool waitForExtEventOrTimeout();
    
    /**
     * Wait for the interrupt.
     *  You must have called setAbsoluteWakeupWait() before this.
     */
    void wait();
    
    /**
     * Set the timer timer interrupt to occur at an absolute value
     * \param value absolute value when the interrupt will occur
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
