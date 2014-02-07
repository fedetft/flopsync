 
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
#include "monotonic_clock.h"
#include <algorithm>

using namespace std;

//
// class MonotonicClock
//

unsigned long long MonotonicClock::rootFrame2localAbsolute(unsigned int root)
{
    #ifdef SEND_TIMESTAMPS
    //Can't have a monotonic clock with a sync scheme that overwrites the RTC
    assert(sync.overwritesHardwareClock()==false);
    #endif //SEND_TIMESTAMPS
    int signedRoot=root; //Conversion unsigned to signed is *required*
    int period=nominalPeriod;
    long long correction=signedRoot;
    correction*=sync.getClockCorrection();
    int sign=correction>=0 ? 1 : -1; //Round towards closest
    int dividedCorrection=(correction+(sign*period/2))/period;
    return flood.getComputedFrameStart()+max(0,signedRoot+dividedCorrection);
}