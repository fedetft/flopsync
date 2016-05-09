 
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
#include "controller_flopsync.h"
#include <algorithm>

using namespace std;

//
// class ControllerFlopsync
//

ControllerFlopsync::ControllerFlopsync() { reset(); }

pair<int,int> ControllerFlopsync::computeCorrection(int e)
{
    //u(k)=u(k-1)+1.375*e(k)-e(k-1)
    int u=uo+11*e-8*eo;

    //The controller output needs to be quantized, but instead of simply
    //doing u/8 which rounds towards the lowest number use a slightly more
    //advanced algorithm to round towards the closest one, as when the error
    //is close to +/-1 timer tick this makes a significant difference.
    int sign=u>=0 ? 1 : -1;
    int uquant=(u+4*sign)/8;

    //Adaptive state quantization, while the output always needs to be
    //quantized, the state is only quantized if the error is zero
    uo= e==0 ? 8*uquant : u;
    eo=e;
  
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

pair<int,int> ControllerFlopsync::lostPacket()
{
    //Double receiver window on packet loss, still clamped to max value
    dw=min<int>(2*dw,w/scaleFactor);
    return make_pair(getClockCorrection(),scaleFactor*dw);
}

void ControllerFlopsync::reset()
{
    uo=eo=sum=squareSum=count=0;
    dw=w/scaleFactor;
}

int ControllerFlopsync::getClockCorrection() const
{
    //Error measure is unavailable if the packet is lost, the best we can
    //do is to reuse the past correction value
    int sign=uo>=0 ? 1 : -1;
    return (uo+4*sign)/8;
}