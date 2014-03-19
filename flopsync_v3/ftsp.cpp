 
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
#include "ftsp.h"
#include <cstdio>
#include <cstring>

using namespace std;

//
// class FTSP
//

FTSP::FTSP() { reset(); }

#ifdef SEND_TIMESTAMPS
void FTSP::timestamps(unsigned long long globalTime, unsigned long long localTime)
{
    if(!filling || dex>0)
    {
        //Just to have a measure of e(t(k)-)
        offset=localTime-rootFrame2localAbsolute(globalTime-this->globalTime);
    }
    
    this->globalTime=globalTime;
    this->localTime=localTime;
    
    unsigned long long ovr_local_rtc_base=reg_local_rtcs[dex];
    reg_local_rtcs[dex]=localTime;
    reg_rtc_offs[dex]=(long long)localTime-(long long)globalTime;
    if(filling && dex==0) local_rtc_base=localTime;
    if(!filling) local_rtc_base=ovr_local_rtc_base;
    if(filling) num_reg_data=dex+1;
    else num_reg_data=regression_entries;
    dex++;
    if(filling && dex>=regression_entries) filling=false;
    if(dex>=regression_entries) dex=0;
    
    if(num_reg_data<2)
    {
        a=(long double)localTime-(long double)globalTime;
        b=0;
        printf("+ b=%Le a=%Lf localTime=%llu globalTime=%llu\n",b,a,localTime,globalTime);
    }
}
#endif//SEND_TIMESTAMPS

pair<int,int> FTSP::computeCorrection(int e)
{
    this->e=e;
    
    if(num_reg_data<2) return make_pair(e,w);
    
    long long sum_t=0;
    long long sum_o=0;
    long long sum_ot=0;
    long long sum_t2=0;
    
    for(int i=0;i<num_reg_data;i++)
    {
        long long t=reg_local_rtcs[i]-local_rtc_base;
        long long o=reg_rtc_offs[i];
        sum_t+=t;
        sum_o+=o;
        sum_ot+=o*t;
        sum_t2+=t*t;
    }
    int n=num_reg_data;
    b=(long double)(n*sum_ot-sum_o*sum_t)/(long double)(n*sum_t2-sum_t*sum_t);
    a=((long double)sum_o-b*(long double)sum_t)/(long double)n;
    printf("b=%Le a=%Lf localTime=%llu globalTime=%llu\n",b,a,localTime,globalTime);
    
    return make_pair(e,w);
}

pair<int,int> FTSP::lostPacket()
{
    return make_pair(e,w);
}

void FTSP::reset()
{
    e=offset=0;
    
    dex=0;
    filling=true;
    memset(reg_local_rtcs,0,sizeof(reg_local_rtcs));
    memset(reg_rtc_offs,0,sizeof(reg_rtc_offs));
}

unsigned long long FTSP::rootFrame2localAbsolute(unsigned long long time) const
{
    long double global=globalTime+time;
    long double lastOffset=(long double)localTime-(long double)globalTime;
    long double lastSyncPoint=localTime;
    unsigned long long result=(global+lastOffset-b*lastSyncPoint)/(1.0-b);
    return result;
}