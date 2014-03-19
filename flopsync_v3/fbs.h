 
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
#ifndef FBS_H
#define	FBS_H

#include "protocol_constants.h"
#include "synchronizer.h"
#include "../drivers/timer.h"

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

    #ifdef SEND_TIMESTAMPS
    /**
     * Must be called before computeCorrection() if timestamp transmission is
     * enabled
     * \param globalTime time received in timestamp
     * \param localTime local time when packet received
     */
    void timestamps(unsigned long long globalTime, unsigned long long localTime);

    /**
     * \return true of the synchronization scheme alters the node's hardware
     * clock. In this case, monotonic clocks are impossible to implement.
     */
    bool overwritesHardwareClock() const { return true; }
    #endif//SEND_TIMESTAMPS
    
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

#endif //FBS_H