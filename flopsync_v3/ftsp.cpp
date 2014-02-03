 
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

FTSP::FTSP() : overflowCounterLocal(0), overflowCounterGlobal(0) { reset(); }

void FTSP::timestamps(unsigned int globalTime, unsigned int localTime)
{
    if(!filling || dex>0)
    {
        //Just to have a measure of e(t(k)-)
        offset=rootFrame2localAbsolute(globalTime-this->globalTime);
        offset=(int)localTime-offset;
    }
    
    if(this->localTime>localTime) overflowCounterLocal++;
    if(this->globalTime>globalTime) overflowCounterGlobal++;
    this->globalTime=globalTime;
    this->localTime=localTime;
    
    unsigned long long ovr_local_rtc_base=reg_local_rtcs[dex];
    unsigned long long temp=overflowCounterLocal;
    temp<<=32;
    temp|=localTime;
    reg_local_rtcs[dex]=temp;
    reg_rtc_offs[dex]=(int)localTime-(int)globalTime;
    if(filling && dex==0) local_rtc_base=temp;
    if(!filling) local_rtc_base=ovr_local_rtc_base;
    if(filling) num_reg_data=dex+1;
    else num_reg_data=regression_entries;
    dex++;
    if(filling && dex>=regression_entries) filling=false;
    if(dex>=regression_entries) dex=0;
    
    if(num_reg_data<2)
    {
        a=(double)localTime-(double)globalTime;
        b=0;
        printf("+ b=%e a=%f localTime=%u globalTime=%u\n",b,a,localTime,globalTime);
    }
}

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
    long long n=num_reg_data;
    b=(double)(n*sum_ot-sum_o*sum_t)/(double)(n*sum_t2-sum_t*sum_t);
    a=((double)sum_o-b*(double)sum_t)/(double)n;
    printf("b=%e a=%f localTime=%u globalTime=%u\n",b,a,localTime,globalTime);
    
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

unsigned int FTSP::rootFrame2localAbsolute(unsigned int time) const
{
    unsigned long long temp=overflowCounterGlobal;
    temp<<=32;
    temp|=globalTime;
    temp+=time;
    double global=temp;
    //printf("timeBefore=%u ",temp);
    
//     // local=(global+a-b*local_rtc_base)/(1-b)
//     double lr=local_rtc_base;
//     unsigned long long result=(global+a-b*lr)/(1.0-b);
    
    // local=(global+lastOffset-b*lastSyncPoint)/(1.0-b);
    double lastOffset=(int)localTime-(int)globalTime;
    unsigned long long temp2=overflowCounterLocal;
    temp2<<=32;
    temp2|=localTime;
    double lastSyncPoint=temp2;
    unsigned long long result=(global+lastOffset-b*lastSyncPoint)/(1.0-b);
    
    //printf(" timeAfter=%u\n",result);
    return result & 0xffffffff;
}