/***************************************************************************
 *   Copyright (C)  2013 by Terraneo Federico                              *
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

#ifndef FLOPSYNC2_H
#define	FLOPSYNC2_H

#include <utility>
#include <cstdio>
#include <cmath>
#include "drivers/nrf24l01.h"
#include "drivers/rtc.h"
#include "protocol_constants.h"
#include <miosix.h>

/**
 * Base class from which flooding schemes derive
 */
class FloodingScheme
{
public:
    /**
     * Needs to be periodically called to send/receive the synchronization
     * packet. This member function sleeps till it's time to send the packet,
     * then sends it and returns. If this function isn't called again within
     * nominalPeriod, the synchronization won't work.
     * \return true if the node desynchronized
     */
    virtual bool synchronize()=0;
    
    /**
     * Called if synchronization is lost
     */
    virtual void resynchronize();
    
    /**
     * \return The (local) time when the synchronization packet was
     * actually received
     */
    virtual unsigned int getMeasuredFrameStart() const=0;
    
    /**
     * \return The (local) time when the synchronization packet was
     * expected to be received
     */
    virtual unsigned int getComputedFrameStart() const=0;

    #ifdef SEND_TIMESTAMPS
    /**
     * \return the timestamp received through the radio
     */
    virtual unsigned int getRadioTimestamp() const=0;
    #endif //SEND_TIMESTAMPS

    /**
     * Destructor
     */
    virtual ~FloodingScheme();
};

/**
 * Base class from which synchronization schemes derive
 */
class Synchronizer
{
public:
    #ifdef SEND_TIMESTAMPS
    /**
     * Must be called before computeCorrection() if timestamp transmission is
     * enabled
     * \param globalTime time received in timestamp
     * \param localTime local time when packet received
     */
    virtual void timestamps(unsigned int globalTime, unsigned int localTime);

    /**
     * \return true of the synchronization scheme alters the node's hardware
     * clock. In this case, monotonic clocks are impossible to implement.
     */
    virtual bool overwritesHardwareClock() const;
    #endif //SEND_TIMESTAMPS

    /**
     * Compute clock correction and receiver window given synchronization error
     * \param e synchronization error
     * \return a pair with the clock correction, and the receiver window
     */
    virtual std::pair<int,int> computeCorrection(int e)=0;
    
    /**
     * Compute clock correction and receiver window when a packet is lost
     * \return a pair with the clock correction, and the receiver window
     */
    virtual std::pair<int,int> lostPacket()=0;
    
    /**
     * Used after a resynchronization to reset the controller state
     */
    virtual void reset()=0;
    
    /**
     * \return the synchronization error e(k)
     */
    virtual int getSyncError() const=0;
    
    /**
     * \return the clock correction u(k)
     */
    virtual int getClockCorrection() const=0;
    
    /**
     * \return the receiver window (w)
     */
    virtual int getReceiverWindow() const=0;

    /**
     * Destructor
     */
    virtual ~Synchronizer();
};

/**
 * Base class from which clocks derive
 */
class Clock
{
public:
    /**
     * This member function converts between root frame time, that is, a time
     * that starts goes from 0 to nominalPeriod-1 and is referenced to the root
     * node, to absolute time referenced to the local node time. It can be used
     * to send a packet to the root node at the the frame time it expects it.
     * \param root a frame time (0 to nominalPeriod-1) referenced to the root node
     * \return the local absolute time time corresponding to the given root time
     */
    virtual unsigned int rootFrame2localAbsolute(unsigned int root)=0;

    /**
     * Destructor
     */
    virtual ~Clock();
};

/**
 * A very simple flooding scheme, root node implementation
 */
class FlooderRootNode : public FloodingScheme
{
public:
    /**
     * Constructor
     * \param rtc the main timer
     */
    FlooderRootNode(Timer& rtc);

    /**
     * Needs to be periodically called to send the synchronization packet.
     * This member function sleeps till it's time to send the packet, then
     * sends it and returns. If this function isn't called again within
     * nominalPeriod, the synchronization won't work.
     * \return true if the node desynchronized
     */
    bool synchronize();
    
    /**
     * \return The (local) time when the synchronization packet was
     * actually received
     */
    unsigned int getMeasuredFrameStart() const { return frameStart; }
    
    /**
     * \return The (local) time when the synchronization packet was
     * expected to be received
     */
    unsigned int getComputedFrameStart() const { return frameStart; }
    
    #ifdef SEND_TIMESTAMPS
    /**
     * \return the timestamp received through the radio
     */
    unsigned int getRadioTimestamp() const { return frameStart; }
    #endif //SEND_TIMESTAMPS

private:
    Timer& rtc;
    Nrf24l01& nrf;
    AuxiliaryTimer& timer;
    unsigned int frameStart;
    unsigned int wakeupTime;
};

/**
 * A very simple flooding scheme, synchronized node implementation
 */
class FlooderSyncNode : public FloodingScheme
{
public:
    /**
     * Constructor
     * \param rtc the main timer
     * \param synchronizer pointer to synchronizer.
     * \param hop specifies to which hop we need to attach. 0 is the root node,
     * 1 is the first hop, ... This allows to force a multi hop network even
     * though nodes are in radio range.
     */
    FlooderSyncNode(Timer& rtc, Synchronizer& synchronizer, unsigned char hop=0);

    /**
     * Needs to be periodically called to send the synchronization packet.
     * This member function sleeps till it's time to send the packet, then
     * sends it and returns. If this function isn't called again within
     * nominalPeriod, the synchronization won't work.
     * \return true if the node desynchronized
     */
    bool synchronize();
    
    /**
     * Called if synchronization is lost
     */
    void resynchronize();
    
    /**
     * \return The (local) time when the synchronization packet was
     * actually received
     */
    unsigned int getMeasuredFrameStart() const { return measuredFrameStart; }
    
    /**
     * \return The (local) time when the synchronization packet was
     * expected to be received
     */
    unsigned int getComputedFrameStart() const
    {
        //Correct frame start considering hops
        return computedFrameStart-hop*retransmitDelta;
    }
    
    #ifdef SEND_TIMESTAMPS
    /**
     * \return the timestamp received through the radio
     */
    unsigned int getRadioTimestamp() const
    {
        return receivedTimestamp+packetTime; //Correct for packet lenght
    }
    #endif //SEND_TIMESTAMPS
    
    /**
     * \return true if in this frame the sync packet was not received
     */
    bool isPacketMissed() const { return missPackets>0; }

private:  
    /**
     * Resend the synchronization packet to let other hops synchronize
     */
    void rebroadcast();
    
    Timer& rtc;
    Nrf24l01& nrf;
    AuxiliaryTimer& timer;
    Synchronizer& synchronizer;
    unsigned int measuredFrameStart;
    unsigned int computedFrameStart;
    int clockCorrection;
    int receiverWindow; 
    unsigned char missPackets;
    unsigned char hop;
    #ifdef SEND_TIMESTAMPS
    unsigned int receivedTimestamp;
    #endif //SEND_TIMESTAMPS
    
    static const unsigned char maxMissPackets=3;
};

/**
 * A new flopsync controller that can reach zero steady-state error both
 * with step-like and ramp-like disturbances.
 * It provides better synchronization under temperature changes, that occur
 * so slowly with respect to the controller operation to look like ramp
 * changes in clock skew.
 */
class OptimizedRampFlopsync : public Synchronizer
{
public:
    /**
     * Constructor
     */
    OptimizedRampFlopsync();
    
    /**
     * Compute clock correction and receiver window given synchronization error
     * \param e synchronization error
     * \return a pair with the clock correction, and the receiver window
     */
    std::pair<int,int> computeCorrection(int e);
    
    /**
     * Compute clock correction and receiver window when a packet is lost
     * \return a pair with the clock correction, and the receiver window
     */
    std::pair<int,int> lostPacket();
    
    /**
     * Used after a resynchronization to reset the controller state
     */
    void reset();
    
    /**
     * \return the synchronization error e(k)
     */
    int getSyncError() const { return eo; }
    
    /**
     * \return the clock correction u(k)
     */
    int getClockCorrection() const;
    
    /**
     * \return the receiver window (w)
     */
    int getReceiverWindow() const { return scaleFactor*dw; }
    
private:
    int uo, uoo;
    int sum;
    int squareSum;
    short eo, eoo;
    unsigned char count;
    unsigned char dw;
    char init;
    
    static const int numSamples=64; //Number of samples for variance compuation
    static const int fp=64; //Fixed point, log2(fp) bits are the decimal part
    #ifndef USE_VHT
    static const int scaleFactor=1;
    #else //USE_VHT
    //The maximum value that can enter the window computation algorithm without
    //without causing overflows is around 700, resulting in a scaleFactor of
    //5 when the vht resolution is 1us, and w is 3ms. That however would cause
    //overflow when writing the result to dw, which is just an unsigned char
    //(to save RAM). This requires a higher scale factor, of about w/255, or 12.
    //However, this requires more iterations to approximate the square root,
    //so we're using a scale factor of 30.
    static const int scaleFactor=30;
    #endif //USE_VHT
};

/**
 * Dummy synchronizer that does not perform skew/drift compensation.
 * This works using the difference between expected and actual packet
 * time without needing timestamps in packets, and does not alter the
 * node's hardware clock.
 */
class DummySynchronizer : public Synchronizer
{
public:
    /**
     * Constructor
     */
    DummySynchronizer();
    
    /**
     * Compute clock correction and receiver window given synchronization error
     * \param e synchronization error
     * \return a pair with the clock correction, and the receiver window
     */
    std::pair<int,int> computeCorrection(int e);
    
    /**
     * Compute clock correction and receiver window when a packet is lost
     * \return a pair with the clock correction, and the receiver window
     */
    std::pair<int,int> lostPacket();
    
    /**
     * Used after a resynchronization to reset the controller state
     */
    void reset();
    
    /**
     * \return the synchronization error e(k)
     */
    int getSyncError() const { return 0; }
    
    /**
     * \return the clock correction u(k)
     */
    int getClockCorrection() const { return 0; }
    
    /**
     * \return the receiver window (w)
     */
    int getReceiverWindow() const { return w; }
    
private:
    int uo;
};

#ifdef SEND_TIMESTAMPS
/**
 * Dummy synchronizer that does not perform skew/drift compensation.
 * This works using the received timestamp, and alters the node's
 * hardware clock.
 */
class DummySynchronizer2 : public Synchronizer
{
public:
    /**
     * Constructor
     */
    DummySynchronizer2(Timer& timer);

    /**
     * Must be called before computeCorrection() if timestamp transmission is
     * enabled
     * \param globalTime time received in timestamp
     * \param localTime local time when packet received
     */
    void timestamps(unsigned int globalTime, unsigned int localTime);

    /**
     * \return true of the synchronization scheme alters the node's hardware
     * clock. In this case, monotonic clocks are impossible to implement.
     */
    bool overwritesHardwareClock() const { return true; }
    
    /**
     * Compute clock correction and receiver window given synchronization error
     * \param e synchronization error
     * \return a pair with the clock correction, and the receiver window
     */
    std::pair<int,int> computeCorrection(int e);
    
    /**
     * Compute clock correction and receiver window when a packet is lost
     * \return a pair with the clock correction, and the receiver window
     */
    std::pair<int,int> lostPacket();
    
    /**
     * Used after a resynchronization to reset the controller state
     */
    void reset() { e=0; }
    
    /**
     * \return the synchronization error e(k)
     */
    int getSyncError() const { return e; }
    
    /**
     * \return the clock correction u(k)
     */
    int getClockCorrection() const { return 0; }
    
    /**
     * \return the receiver window (w)
     */
    int getReceiverWindow() const { return w; }
    
private:
    Timer& timer;
    int e;
};

/**
 * A linear regression-based clock synchronization scheme
 */
class FTSP : public Synchronizer
{
public:
    /**
     * Constructor
     */
    FTSP();

    /**
     * Must be called before computeCorrection() if timestamp transmission is
     * enabled
     * \param globalTime time received in timestamp
     * \param localTime local time when packet received
     */
    void timestamps(unsigned int globalTime, unsigned int localTime);

    /**
     * \return true of the synchronization scheme alters the node's hardware
     * clock. In this case, monotonic clocks are impossible to implement.
     */
    bool overwritesHardwareClock() const { return false; }
    
    /**
     * Compute clock correction and receiver window given synchronization error
     * \param e synchronization error
     * \return a pair with the clock correction, and the receiver window
     */
    std::pair<int,int> computeCorrection(int e);
    
    /**
     * Compute clock correction and receiver window when a packet is lost
     * \return a pair with the clock correction, and the receiver window
     */
    std::pair<int,int> lostPacket();
    
    /**
     * Used after a resynchronization to reset the controller state
     */
    void reset();
    
    /**
     * \return the synchronization error e(k)
     */
    int getSyncError() const { return offset; }
    
    /**
     * \return the clock correction u(k)
     */
    int getClockCorrection() const { return 0; }
    
    /**
     * \return the receiver window (w)
     */
    int getReceiverWindow() const { return w; }
    
    unsigned int rootFrame2localAbsolute(unsigned int time) const;
    
private:
    unsigned int globalTime,localTime;
    unsigned int overflowCounterLocal,overflowCounterGlobal; 
    int e,offset;
    
    static const int regression_entries=8;
    int dex;
    int num_reg_data;
    bool filling;
    unsigned long long reg_local_rtcs[regression_entries];
    int reg_rtc_offs[regression_entries];
    unsigned long long local_rtc_base;
    double a,b;  
};

/**
 * A PI controller feeded with timestamps
 */
class FBS : public Synchronizer
{
public:
    /**
     * Constructor
     */
    FBS(Timer& timer);

    /**
     * Must be called before computeCorrection() if timestamp transmission is
     * enabled
     * \param globalTime time received in timestamp
     * \param localTime local time when packet received
     */
    void timestamps(unsigned int globalTime, unsigned int localTime);

    /**
     * \return true of the synchronization scheme alters the node's hardware
     * clock. In this case, monotonic clocks are impossible to implement.
     */
    bool overwritesHardwareClock() const { return true; }
    
    /**
     * Compute clock correction and receiver window given synchronization error
     * \param e synchronization error
     * \return a pair with the clock correction, and the receiver window
     */
    std::pair<int,int> computeCorrection(int e);
    
    /**
     * Compute clock correction and receiver window when a packet is lost
     * \return a pair with the clock correction, and the receiver window
     */
    std::pair<int,int> lostPacket();
    
    /**
     * Used after a resynchronization to reset the controller state
     */
    void reset();
    
    /**
     * \return the synchronization error e(k)
     */
    int getSyncError() const { return offset; }
    
    /**
     * \return the clock correction u(k)
     */
    int getClockCorrection() const { return u; }
    
    /**
     * \return the receiver window (w)
     */
    int getReceiverWindow() const { return w; }
    
private:
    static const float kp=0.7847f;//0.35f;
    static const float ki=0.7847f;//0.05f;
    
    Timer& timer;
    int offset,offseto;
    float u;
    bool first;
};
#endif //SEND_TIMESTAMPS

/**
 * A clock that is guaranteed to be monotonic
 */
class MonotonicClock : public Clock
{
public:
    /**
     * Constructor
     * \param fs the flopsynch controller
     * \param flood the flooding scheme
     */
    MonotonicClock(const Synchronizer& sync, const FloodingScheme& flood)
        : sync(sync), flood(flood) {}
    
    /**
     * This member function converts between root frame time, that is, a time
     * that starts goes from 0 to nominalPeriod-1 and is referenced to the root
     * node, to absolute time referenced to the local node time. It can be used
     * to send a packet to the root node at the the frame time it expects it.
     * \param root a frame time (0 to nominalPeriod-1) referenced to the root node
     * \return the local absolute time time corresponding to the given root time
     */
    unsigned int rootFrame2localAbsolute(unsigned int root);
    
private:
    const Synchronizer& sync;
    const FloodingScheme& flood;
};

/**
 * A non-monotonic clock
 */
class NonMonotonicClock : public Clock
{
public:
    /**
     * Constructor
     * \param fs the flopsynch controller
     * \param flood the flooding scheme
     */
    NonMonotonicClock(const Synchronizer& sync, const FloodingScheme& flood)
        : sync(sync), flood(flood) {}
    
    /**
     * This member function converts between root frame time, that is, a time
     * that starts goes from 0 to nominalPeriod-1 and is referenced to the root
     * node, to absolute time referenced to the local node time. It can be used
     * to send a packet to the root node at the the frame time it expects it.
     * \param root a frame time (0 to nominalPeriod-1) referenced to the root node
     * \return the local absolute time time corresponding to the given root time
     */
    unsigned int rootFrame2localAbsolute(unsigned int root);
    
private:
    const Synchronizer& sync;
    const FloodingScheme& flood;
};

/**
 * To send (in the root node) or rebroadcast (in the synchronized nodes) the
 * synchronization packet with a jitter as low as possible, disable the systick
 * interrupt, that in Miosix is used for preemption. Note that it's not
 * possible to simply disable all interrupts as the radio code is
 * interrupt-driven.
 * This is a bit of a hack, and may break in future Miosix versions.
 */
class CriticalSection
{
public:
    /**
     * Constructor
     */
    CriticalSection()
    {
        SysTick->CTRL=0;
    }
    
    /**
     * Destructor
     */
    ~CriticalSection()
    {
        SysTick->CTRL=SysTick_CTRL_ENABLE
                    | SysTick_CTRL_TICKINT
                    | SysTick_CTRL_CLKSOURCE;
    }
};

#endif //FLOPSYNC2_H
