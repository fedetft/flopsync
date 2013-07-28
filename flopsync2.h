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

#include "drivers/nrf24l01.h"
#include "drivers/rtc.h"
#include "flopsync_controllers.h"
#include "variance_estimator.h"

/**
 * Base class from which flooding schemes derive
 */
class FloodingScheme
{
public:
    /**
     * Needs to be periodically called to send the synchronizzation packet.
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
};

/**
 * Flopsync implementation for the root node
 */
class FlooderRootNode : public FloodingScheme
{
public:
    FlooderRootNode();

    /**
     * Needs to be periodically called to send the synchronizzation packet.
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


class FlooderSyncNode : public FloodingScheme
{
public:
    FlooderSyncNode();

    /**
     * Needs to be periodically called to send the synchronizzation packet.
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

private:    
    Rtc& rtc;
    Nrf24l01& nrf;
    AuxiliaryTimer& timer;
    unsigned int measuredFrameStart;
    unsigned int computedFrameStart;
    short clockCorrection;
    unsigned char receiverWindow; 
    unsigned char missPackets;
    VarianceEstimator ve;
    Controller4 controller;
    
    static const unsigned char maxMissPackets=3;
};

#endif //FLOPSYNC2_H
