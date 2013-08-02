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

#ifndef RTC_H
#define	RTC_H

/**
 * Manages the auxiliary timer used to handle window timeouts.
 * Note: there are ways to avoid using a second timer for the timeout, and
 * reuse the same RTC, especially when it is of the absolute type, but for
 * simplicity the timeout computation has been implemented in this way. 
 */
class AuxiliaryTimer
{
public:
    /**
     * \return an instance to the auxiliary timer (singleton)
     */
    static AuxiliaryTimer& instance();
    
    /**
     * Initializes the synchronization window timeout timer
     * \param ctr timeout value in 1/16384 of a second. If zero, the timeout
     * is disabled, and this function waits indefinitely till the nRF asserts
     * the IRQ signal.
     */
    void initTimeoutTimer(unsigned short ctr);

    /**
     * Return when either a packet arrived or the timeout expired
     * \return true if timeout occurred, false if packet arrived
     */
    bool waitForPacketOrTimeout();
    
    /**
     * Wait for the timeout only
     */
    void waitForTimeout();

    /**
     * Stop the timeout timer
     */
    void stopTimeoutTimer();
    
private:
    /**
     * Constructor
     */
    AuxiliaryTimer();
};

/**
 * Manages the 32KHz hardware timer that runs also in low power mode
 */
class Rtc
{
public:
    /**
     * \return an instance to the RTC (singleton)
     */
    static Rtc& instance();
    
    /**
     * \return the RTC counter value
     */
    unsigned int getValue() const;
    
    /**
     * Set the RTC timer interrupt to occur at an absolute value
     * \param value absolute value when the interrupt will occur
     */
    void setAbsoluteWakeup(unsigned int value);
    
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
     * Wait for the interrupt
     */
    void wait();
    
    /**
     * Puts the microcontroller in low power mode (few uA) and wait for the
     * RTC interrupt
     */
    void sleepAndWait();
    
private:
    /**
     * Constructor
     */
    Rtc();
};

/**
 * Virtual high resolution timer
 */
class VHT
{
public:
    /**
     * \return an instance to the VHT (singleton)
     */
    static VHT& instance();
    
    /**
     * Synchronize the VHT timer with the low frequency RTC
     */
    void synchronizeWith(Rtc& rtc);
    
    /**
     * \return the precise time when the IRQ signal of the nRF24L01 was asserted
     */
    unsigned int getPacketTimestamp();
    
    /**
     * Wait until a specified time
     * \param time a 16 bit unsigned value in VHT time units
     */
    void waitUntil(unsigned short time);
    
private:
    /**
     * Constructor
     */
    VHT();
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

#endif //RTC_H
