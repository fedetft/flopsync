 
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
#ifndef FLOODER_SYNC_NODE_H
#define	FLOODER_SYNC_NODE_H

#include "../drivers/transceiver.h"
#include "../drivers/rtc.h"
#include "synchronizer.h"
#include "protocol_constants.h"
#include "flooding_scheme.h"
#include <miosix.h>

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
    Transceiver& tr;
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

#endif //FLOODER_SYNC_NODE_H