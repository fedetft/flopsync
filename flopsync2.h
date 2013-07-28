/***************************************************************************
 *   Copyright (C) 2012, 2013 by Terraneo Federico                         *
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

/**
 * Base class from which flooding schemes derive
 */
class FloodingScheme
{
public:
    /**
     * Needs to be periodically called to send the synchronization packet.
     * This member function sleeps till it's time to send the packet, then
     * sends it and returns. If this function isn't called again within
     * nominalPeriod, the synchronization won't work.
     * \return true if the node desynchronized
     */
    virtual bool synchronize()=0;
    
    /**
     * Called if synchronization is lost
     */
    virtual void resynchronize();
    
    /**
     * \return The (local) time when the current frame has started.
     */
    virtual unsigned int getFrameStart() const=0;
    
    /**
     * \return The clock correction computed by the synchronization controller
     */
    virtual unsigned int getClockCorrection() const;
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
    virtual std::pair<short,unsigned char> computeCorrection(short e)=0;
    
    /**
     * Compute clock correction and receiver window when a packet is lost
     * \return a pair with the clock correction, and the receiver window
     */
    virtual std::pair<short,unsigned char> lostPacket()=0;
    
    /**
     * Used after a resynchronization to reset the controller state
     */
    virtual void reset()=0;
};

/**
 * A very simple flooding scheme, root node implementation
 */
class FlooderRootNode : public FloodingScheme
{
public:
    /**
     * Constructor
     */
    FlooderRootNode();

    /**
     * Needs to be periodically called to send the synchronization packet.
     * This member function sleeps till it's time to send the packet, then
     * sends it and returns. If this function isn't called again within
     * nominalPeriod, the synchronization won't work.
     * \return true if the node desynchronized
     */
    bool synchronize();
    
    /**
     * \return The (local) time when the current frame has started.
     */
    unsigned int getFrameStart() const { return frameStart; }

private:
    Rtc& rtc;
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
     * \param synchronizer pointer to synchronizer. The caller owns the pointer.
     * \param hop specifies to which hop we need to attach. 0 is the root node,
     * 1 is the first hop, ... This allows to force a multi hop network even
     * though nodes are in radio range.
     */
    FlooderSyncNode(Synchronizer *synchronizer, unsigned char hop=0);

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
     * \return The (local) time when the current frame has started.
     */
    unsigned int getFrameStart() const { return measuredFrameStart; }
    
    /**
     * \return The clock correction computed by the synchronization controller
     */
    virtual unsigned int getClockCorrection() const { return clockCorrection; }

private:  
    /**
     * Resend the synchronization packet to let other hops synchronize
     */
    void rebroadcast();
    
    Rtc& rtc;
    Nrf24l01& nrf;
    AuxiliaryTimer& timer;
    Synchronizer *synchronizer;
    unsigned int measuredFrameStart;
    unsigned int computedFrameStart;
    short clockCorrection;
    unsigned char receiverWindow; 
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
    virtual std::pair<short,unsigned char> computeCorrection(short e);
    
    /**
     * Compute clock correction and receiver window when a packet is lost
     * \return a pair with the clock correction, and the receiver window
     */
    virtual std::pair<short,unsigned char> lostPacket();
    
    /**
     * Used after a resynchronization to reset the controller state
     */
    virtual void reset();
    
private:
    short uo;
    short eo;
    int sum;
    int squareSum;
    unsigned char count;
    unsigned char var;
    
    static const int numSamples=64; //Number of samples for variance compuation
    static const int fp=64; //Fixed point, log2(fp) bits are the decimal part
};

#endif //FLOPSYNC2_H
