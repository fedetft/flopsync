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
#include "drivers/nrf24l01.h"
#include "drivers/rtc.h"
#include "protocol_constants.h"

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
};

/**
 * Base class from which synchronization schemes derive
 */
class Synchronizer
{
public:
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
     * \return the previous clock correction u(k-1)
     */
    virtual int getPreviousClockCorrection() const=0;
    
    /**
     * \return the receiver window (w)
     */
    virtual int getReceiverWindow() const=0;
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
    
    static const unsigned char maxMissPackets=3;
};

/**
 * This class is basically Controller4 from flopsync_controllers.h plus
 * VarianceEstimator with a Synchronizer compatible interface and some
 * small RAM usage improvements
 */
class OptimizedFlopsync : public Synchronizer
{
public:
    /**
     * Constructor
     */
    OptimizedFlopsync();
    
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
     * \return the previous clock correction u(k-1)
     */
    int getPreviousClockCorrection() const { return uoo; }
    
    /**
     * \return the receiver window (w)
     */
    int getReceiverWindow() const { return var; }
    
private:
    int uo;
    int uoo; //Used by non-monotonic clock
    int sum;
    int squareSum;
    short eo;
    unsigned char count;
    unsigned char var;
    
    static const int numSamples=64; //Number of samples for variance compuation
    static const int fp=64; //Fixed point, log2(fp) bits are the decimal part
};

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

#endif //FLOPSYNC2_H
