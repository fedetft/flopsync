 
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
#include "optimized_ramp_flopsync.h"
#include <algorithm>

using namespace std;
//
// class OptimizedRampFlopsync
//

OptimizedRampFlopsync::OptimizedRampFlopsync() { reset(); }

pair<int,int> OptimizedRampFlopsync::computeCorrection(int e)
{
    switch(init)
    {
        case 0:
            init=1;
            eo=e;
            uo=2*64*e;
            uoo=64*e;
            return make_pair(2*e,w); //One step of a deadbeat controller
        case 1:
            init=2;
            eo=0;
            uo/=2;
    }
    
    //alpha=3/8
    //u(k)=2u(k-1)-u(k-2)+2.25e(k)-2.859375e(k-1)+e(k-2) with values kept multiplied by 64
    int u=2*uo-uoo+144*e-183*eo+64*eoo;
    uoo=uo;
    uo=u;
    eoo=eo;
    eo=e;

    int sign=u>=0 ? 1 : -1;
    int uquant=(u+32*sign)/64;
    
    //Scale numbers if VHT is enabled to prevent overflows
    int wMax=w/scaleFactor;
    int wMin=minw/scaleFactor;
    e/=scaleFactor;
    
    //Update receiver window size
    sum+=e*fp;
    squareSum+=e*e*fp;
    if(++count>=numSamples)
    {
        //Variance computed as E[X^2]-E[X]^2
        int average=sum/numSamples;
        int var=squareSum/numSamples-average*average/fp;
        //Using the Babylonian method to approximate square root
        int stddev=var/7;
        for(int j=0;j<3;j++) if(stddev>0) stddev=(stddev+var*fp/stddev)/2;
        //Set the window size to three sigma
        int winSize=stddev*3/fp;
        //Clamp between min and max window
        dw=max<int>(min<int>(winSize,wMax),wMin);
        sum=squareSum=count=0;
    }

    return make_pair(uquant,scaleFactor*dw);
}

pair<int,int> OptimizedRampFlopsync::lostPacket()
{
    if(init==1)
    {
        init=2;
        eo=0;
        uo/=2;
    }
    //Double receiver window on packet loss, still clamped to max value
    dw=min<int>(2*dw,w/scaleFactor);
    return make_pair(getClockCorrection(),scaleFactor*dw);
}

void OptimizedRampFlopsync::reset()
{
    uo=uoo=eo=eoo=sum=squareSum=count=0;
    dw=w/scaleFactor;
    init=0;
}

int OptimizedRampFlopsync::getClockCorrection() const
{
    //Error measure is unavailable if the packet is lost, the best we can
    //do is to reuse the past correction value
    int sign=uo>=0 ? 1 : -1;
    return (uo+32*sign)/64;
}