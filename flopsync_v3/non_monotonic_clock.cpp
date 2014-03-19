 
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
#include "non_monotonic_clock.h"
#include <algorithm>

using namespace std;

//
// class NonMonotonicClock
//

unsigned long long NonMonotonicClock::localTime(unsigned long long slotGlobalTime)
{
    long long signedSlotGlobalTime=slotGlobalTime; //Conversion unsigned to signed is *required*
    long long T=nominalPeriod;
    long long correction=signedSlotGlobalTime;
    #ifndef SEND_TIMESTAMPS
    correction*=sync.getClockCorrection()-sync.getSyncError();
    #else //SEND_TIMESTAMPS
    if(sync.overwritesHardwareClock()) correction*=sync.getClockCorrection();
    else correction*=sync.getClockCorrection()-sync.getSyncError();
    #endif //SEND_TIMESTAMPS
    int sign=correction>=0 ? 1 : -1; //Round towards closest
    long long dividedCorrection=(correction+(sign*T/2))/T;
    #ifndef SEND_TIMESTAMPS
    return flood.getMeasuredFrameStart()+max(0ll,signedSlotGlobalTime+dividedCorrection);
    #else //SEND_TIMESTAMPS
    const FTSP *ftsp=dynamic_cast<const FTSP*>(&sync);
    if(ftsp) return ftsp->rootFrame2localAbsolute(slotGlobalTime);
    
    return (sync.overwritesHardwareClock() ?
        flood.getRadioTimestamp() : flood.getMeasuredFrameStart()) +
        max(0ll,signedSlotGlobalTime+dividedCorrection);
    #endif //SEND_TIMESTAMPS
    
    //return flood.getMeasuredFrameStart()+slotGlobalTime;
}

unsigned long long NonMonotonicClock::globalTime(unsigned long long slotLocalTime)
{
    //TODO
    return 0;
}

