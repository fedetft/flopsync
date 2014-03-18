 
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
#include "fbs.h"
#include <cstdio>
using namespace std;
//
// class FBS
//

FBS::FBS(Timer& timer) : timer(timer) { reset(); }

#ifdef SEND_TIMESTAMPS
void FBS::timestamps(unsigned long globalTime, unsigned long long localTime)
{
    timer.setValue(globalTime+overwriteClockTime);
    if(!first) offset=(int)localTime-(int)globalTime-u;
    first=false;
}
#endif

std::pair<int,int> FBS::computeCorrection(int e)
{
    u=u+kp*(float)offset+(ki-kp)*(float)offseto;
    offseto=offset;
    printf("FBS offset=%d u=%d\n",offset,(int)u);
    return make_pair(u,w);
}

std::pair<int,int> FBS::lostPacket()
{
    return make_pair(u,w);
}

void FBS::reset()
{
    offset=offseto=0;
    u=0.0f;
    first=true;
}