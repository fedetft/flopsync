/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
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

#include "cc2520_mapping.h"
#include "cc2520.h"
#include "rtc.h"
#include <miosix.h>
#include <cstring>
#include "../test_cc2520/test_config.h"

#ifdef DEBUG_CC2520_MAP
    #include <cstdio>
#endif //DEBUG_CC2520_MAP

using namespace miosix;

//
// class Cc2520Mapping
//

Cc2520Mapping::Cc2520Mapping() : cc2520(Cc2520::instance()), length(16)
{
}

void Cc2520Mapping::setMode(Mode mode)
{
    switch(mode)
    {
        case TX:
            cc2520.setMode(Cc2520::TX);
            break;
        case RX:
            cc2520.setMode(Cc2520::RX);
            break;
        case SLEEP:
            cc2520.setMode(Cc2520::SLEEP);
            break;
    }
            
    
}

void Cc2520Mapping::setFrequency(unsigned short freq)
{
    cc2520.setFrequency(freq);
}

void Cc2520Mapping::setPacketLength(unsigned short length)
{
    if(length<1 || length>32) return;
    this->length=length;
}

void Cc2520Mapping::setAddress(const unsigned char address[3])
{
    
}

bool Cc2520Mapping::irq() const
{
    if(cc2520.getMode() == Cc2520::TX)
    {
        return cc2520.isTxFrameDone();
    }else if(cc2520.getMode() == Cc2520::RX)
    {
        return cc2520.isRxBufferNotEmpty();
    }
    return false;
}

void Cc2520Mapping::writePacket(const void *packet)
{
    
    const unsigned char *data=reinterpret_cast<const unsigned char*>(packet);
    /**
    if(cc2520.writeFrame((unsigned char)this->length,data)==0)
    {
        cc2520.sendTxFifoFrame(); //Send packet
    }*/
    Frame f(length,false,true);
    if (!f.setPayload(data)) return; 
    if(cc2520.writeFrame(f)==0)
    {
        cc2520.sendTxFifoFrame(); //Send packet
    }
}

//Come lo mappo?La TX fifo del cc2520 può contenere un solo frame per volta
bool Cc2520Mapping::isTxSlotAvailable() const
{
    return cc2520.isTxFrameDone();
}

//Come la mappo? La TX fifo del cc2520 può contenere un solo frame per volta
bool Cc2520Mapping::allPacketsSent() const
{
    return true;
}

void Cc2520Mapping::clearTxIrq()
{
    cc2520.isTxFrameDone();
}

void Cc2520Mapping::endWritePacket()
{
    cc2520.isTxFrameDone();
}

void Cc2520Mapping::startReceiving()
{
    //questo metodo non è necessario col CC2520
}

void Cc2520Mapping::readPacket(void *packet)
{
    unsigned char *data=reinterpret_cast<unsigned char*>(packet);
    /**
    unsigned char length = (unsigned char)this->length;
    short int res = cc2520.readFrame(length, data);
    #ifdef DEBUG_CC2520_MAP
        printf("La lunghezza del frame è:%x \n",length);
        printf("readFrame return:%d \n",res);
    #endif //DEBUG_CC2520_MAP
     */
    
    Frame f(false);
    short int res = cc2520.readFrame(f);
    if(!f.getPayload(data)) return;
    #ifdef DEBUG_CC2520_MAP
        printf("readFrame return:%d \n",res);
    #endif //DEBUG_CC2520_MAP
}

bool Cc2520Mapping::isRxPacketAvailable() const
{
    return cc2520.isRxFrameDone();
}