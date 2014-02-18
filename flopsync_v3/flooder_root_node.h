 
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
#ifndef FLOODER_ROOT_NODE_H
#define	FLOODER_ROOT_NODE_H

#include "../drivers/cc2520.h"
#include "../drivers/timer.h"
#include "synchronizer.h"
#include "protocol_constants.h"
#include "flooding_scheme.h"
#include <miosix.h>

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
    FlooderRootNode(Timer& timer);

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
    unsigned long long getMeasuredFrameStart() const { return frameStart; }
    
    /**
     * \return The (local) time when the synchronization packet was
     * expected to be received
     */
    unsigned long long getComputedFrameStart() const { return frameStart; }
    
    #ifdef SEND_TIMESTAMPS
    /**
     * \return the timestamp received through the radio
     */
    unsigned int getRadioTimestamp() const { return frameStart; }
    #endif //SEND_TIMESTAMPS

    ~FlooderRootNode();

    
private:
    Timer& timer;
    Cc2520& transceiver;
    unsigned long long frameStart;
    unsigned long long wakeupTime;
    Frame *syncFrame;
};

#endif //FLOODER_ROOT_NODE_H