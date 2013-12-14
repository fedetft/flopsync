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

#include "cc2520.h"
#include "cc2520_config.h"
#include "rtc.h"
#include <miosix.h>
#include <cstring>
#include "../test_cc2520/test_config.h"

#ifdef DEBUG_CC2520
    #include <cstdio>
#endif //DEBUG_CC2520

using namespace miosix;

//
// class Cc2520
//

Cc2520& Cc2520::instance()
{
    static  Cc2520 singleton;
    return singleton;
}

Cc2520::Cc2520() : mode(DEEP_SLEEP), autoFCS(true)
{
    cc2520GpioInit();
    setMode(DEEP_SLEEP);  //entry state of FSM
}

void Cc2520::setMode(Mode mode)
{
    unsigned char status;
    switch(mode)
    {
        case TX: 
            switch(this->mode)
            {
                case TX:
                    break;
                case RX:
                    status = commandStrobe(CC2520_INS_SRFOFF); //radio in IDLE
                    isExcRaised(CC2520_EXC_USAGE_ERROR, status);
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    commandStrobe(CC2520_INS_SFLUSHTX); //flush TX FIFO
                    break;
                case SLEEP:
                    commandStrobe(CC2520_INS_SXOSCON); //turn on crystal oscillator
                    while ((readStatus() & CC2520_STATUS_XOSC) != CC2520_STATUS_XOSC);

                    commandStrobe(CC2520_INS_SFLUSHTX); //flush TX FIFO
                    break;
                case DEEP_SLEEP:
                    //reset device whit RESETn that automatically start crystal osc.
                    cc2520::reset::low(); //take low for 0.2ms     
                    cc2520::vreg::high();
                    delayUs(200);
                    cc2520::reset::high();
                    //wait until MISO=1xxxxxxx (clock stable and running)
                    while ((readStatus() & CC2520_STATUS_XOSC) != CC2520_STATUS_XOSC);

                    initConfigureReg();

                    commandStrobe(CC2520_INS_SFLUSHTX); //flush TX FIFO
                    break;
                case IDLE:
                    commandStrobe(CC2520_INS_SFLUSHTX); //flush TX FIFO
                    break;
            }
            break;
        case RX: 
            switch(this->mode)
            {
                case TX:
                    status = commandStrobe(CC2520_INS_SFLUSHRX); //flush RX FIFO
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    status = commandStrobe(CC2520_INS_SRXON); //Receive mode
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    break;
                case RX:
                    break;
                case SLEEP:
                    status = commandStrobe(CC2520_INS_SFLUSHRX); //flush RX FIFO
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    status = commandStrobe(CC2520_INS_SXOSCON); //turn on crystal oscillator
                    isExcRaised(CC2520_EXC_OPERAND_ERROR, status);
                    while ((readStatus() & CC2520_STATUS_XOSC) != CC2520_STATUS_XOSC);

                    status = commandStrobe(CC2520_INS_SRXON); //Receive mode
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    break;
                case DEEP_SLEEP :
                    //reset device whit RESETn that automatically start crystal osc.
                    cc2520::reset::low(); //take low for 0.2ms     
                    cc2520::vreg::high();
                    delayUs(200);
                    cc2520::reset::high();
                    //wait until MISO=1xxxxxxx (clock stable and running)
                    while ((readStatus() & CC2520_STATUS_XOSC) != CC2520_STATUS_XOSC);

                    initConfigureReg();
                    status = commandStrobe(CC2520_INS_SFLUSHRX); //flush RX FIFO
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    status = commandStrobe(CC2520_INS_SRXON); //Receive mode
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    break;
                case IDLE:
                    status = commandStrobe(CC2520_INS_SFLUSHRX); //flush RX FIFO
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    status = commandStrobe(CC2520_INS_SRXON); //Receive mode
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    break;
            }
            break;
        case SLEEP: 
            switch(this->mode)
            {
                case TX:
                    status = commandStrobe(CC2520_INS_SRFOFF); //radio in IDLE
                    isExcRaised(CC2520_EXC_USAGE_ERROR, status);
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status); 
                    //Turn off the crystal oscillator 
                    status = commandStrobe(CC2520_INS_SXOSCOFF);
                    isExcRaised(CC2520_EXC_USAGE_ERROR, status);
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    break;
                case RX:
                    status = commandStrobe(CC2520_INS_SRFOFF); //radio in IDLE
                    isExcRaised(CC2520_EXC_USAGE_ERROR, status);
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status); 
                    //Turn off the crystal oscillator 
                    status = commandStrobe(CC2520_INS_SXOSCOFF);
                    isExcRaised(CC2520_EXC_USAGE_ERROR, status);
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    break;
                case SLEEP:
                    break;
                case DEEP_SLEEP :
                    break;
                case IDLE:
                    //Turn off the crystal oscillator ()
                    status = commandStrobe(CC2520_INS_SXOSCOFF);
                    isExcRaised(CC2520_EXC_USAGE_ERROR, status);
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    break;
            }
            break;
        case DEEP_SLEEP: 
            //reset device will remove side effect 
            cc2520::reset::low(); //take low for 0.1ms
            delayUs(100);
            cc2520::cs::high();        
            cc2520::vreg::low();
            cc2520::reset::high();
            break;  
        case IDLE: 
            switch(this->mode)
            {
                case TX:
                    status = commandStrobe(CC2520_INS_SRFOFF); //radio in IDLE
                    isExcRaised(CC2520_EXC_USAGE_ERROR, status);
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    break;
                case RX:
                    status = commandStrobe(CC2520_INS_SRFOFF); //radio in IDLE
                    isExcRaised(CC2520_EXC_USAGE_ERROR, status);
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    break;
                case SLEEP:
                    commandStrobe(CC2520_INS_SXOSCON);
                    while ((readStatus() & CC2520_STATUS_XOSC) != CC2520_STATUS_XOSC);
                    break;
                case DEEP_SLEEP :
                    //reset device whit RESETn that automatically start crystal osc.
                    cc2520::reset::low(); //take low for 0.2ms     
                    cc2520::vreg::high();
                    delayUs(200);
                    cc2520::reset::high();
                    //wait until MISO=1xxxxxxx (clock stable and running)
                    while ((readStatus() & CC2520_STATUS_XOSC) != CC2520_STATUS_XOSC);
                    initConfigureReg();
                    break;
                case IDLE:
                    break;
            }
            break;  
    }
    this->mode=mode;
    #ifdef DEBUG_CC2520
        printf("Setting mode: %d\n",this->mode);
    #endif //DEBUG_CC2520
    return;
}
    
void Cc2520::setFrequency(unsigned short freq)
{
    if(freq<2394 || freq>2507) return;
    this->frequency = freq;
    if(this->mode != SLEEP || this->mode != DEEP_SLEEP) 
                        writeReg(CC2520_FREQCTRL,freq-2394);
}

void Cc2520::setFrequencyChannel(unsigned char channel)
{
    if(channel<11 || channel>26) return;
    Cc2520::setFrequency(2405 + 5*(channel-11));
}

void Cc2520::setShortAddress(const unsigned char address[2])
{
    memcpy(this->shortAddress,address,2);
    if(this->mode != SLEEP || this->mode != DEEP_SLEEP){
    cc2520::cs::low();
    delayUs(1);
    
    //little endian
    cc2520SpiSendRecv(CC2520_INS_MEMORY_WRITE | 0x3);
    cc2520SpiSendRecv(0xf4);
    cc2520SpiSendRecv(address[1]);
    cc2520SpiSendRecv(CC2520_INS_MEMORY_WRITE | 0x3);
    cc2520SpiSendRecv(0xf5);
    cc2520SpiSendRecv(address[0]);
    
    cc2520::cs::high();
    delayUs(1);
    }
}

void Cc2520::setPanId(const unsigned char panId[])
{
    memcpy(this->panId,panId,2);
    if (this->mode == SLEEP || this->mode == DEEP_SLEEP) 
    {
        cc2520::cs::low();
        delayUs(1);

        //little endian
        cc2520SpiSendRecv(CC2520_INS_MEMORY_WRITE | 0x3);
        cc2520SpiSendRecv(0xf2);
        cc2520SpiSendRecv(panId[1]);
        cc2520SpiSendRecv(CC2520_INS_MEMORY_WRITE | 0x3);
        cc2520SpiSendRecv(0xf3);
        cc2520SpiSendRecv(panId[0]);

        cc2520::cs::high();
        delayUs(1);
    }
}

int Cc2520::writeFrame(unsigned char length, const unsigned char* pframe)
{
    if(pframe==NULL) return -2;
    if (length<1 || length>127 ) return -1; 
    cc2520::cs::low();
    delayUs(1);
    unsigned char status = cc2520SpiSendRecv(CC2520_INS_TXBUF);
    cc2520SpiSendRecv(length+2*autoFCS); //adding 2 bytes of FCS
    for(int i=0; i<length; i++) cc2520SpiSendRecv(pframe[i]);
    cc2520::cs::high();
    delayUs(1);
    if(isExcRaised(CC2520_EXC_TX_OVERFLOW,status)) return 1;
    return 0;
}

int Cc2520::writeFrame(const Frame& frame)
{
    if(frame.isNotInit()) return -1; //invalid parameter
    cc2520::cs::low();
    delayUs(1);
    unsigned char status = cc2520SpiSendRecv(CC2520_INS_TXBUF);
    Frame::ConstIterator i; 
    Frame::ConstIterator end = frame.begin();
    for(i=frame.begin(); i<end; i++) cc2520SpiSendRecv(*i);
    cc2520::cs::high();
    delayUs(1);
    if(isExcRaised(CC2520_EXC_TX_OVERFLOW,status)) return 1; //exc tx overflow
    return 0; //ok
}

int Cc2520::readFrame(unsigned char& length, unsigned char* pframe) const
{
    short int result = 0;
    if(pframe==NULL) return -2;
    if (length<1 || length>127 ) return -1; 
    unsigned char currLen;
    cc2520::cs::low();
    delayUs(1);
    unsigned char status = cc2520SpiSendRecv(CC2520_INS_RXBUF);
    currLen = cc2520SpiSendRecv()-2*autoFCS;
    if(currLen>length)
    {
        for(int i=0; i<currLen; i++) 
            (currLen<=length) ? pframe[i]=cc2520SpiSendRecv() 
                                                : cc2520SpiSendRecv();
        result = 0;
    }else
    {
        for(int i=0; i<currLen; i++) pframe[i]=cc2520SpiSendRecv();
        length = currLen;
        result = 1;
    }
    //read the two byte FCS
    unsigned char fcs[2];
    if(autoFCS)
    {
        fcs[0] = cc2520SpiSendRecv();
        fcs[1] = cc2520SpiSendRecv();
    }
    cc2520::cs::high();
    delayUs(1);
    
    if(isExcRaised(CC2520_EXC_RX_UNDERFLOW,status)) return -3;
    if(autoFCS)
        if (!(fcs[1] & 0x80)==0x80 )  return 2;  //fcs incorrect
    return result; 
}

int Cc2520::readFrame(Frame& frame) const
{
    if(!frame.isNotInit() || frame.isAutoFCS()!=autoFCS) return -1;
    cc2520::cs::low();
    delayUs(1);
    unsigned char status = cc2520SpiSendRecv(CC2520_INS_RXBUF);
    frame.initFrame(cc2520SpiSendRecv());
    Frame::Iterator i;
    Frame::Iterator end = frame.end();
    for(i = frame.begin(); i<end; i++) *i = cc2520SpiSendRecv();
    cc2520::cs::high();
    delayUs(1);
    if(isExcRaised(CC2520_EXC_RX_UNDERFLOW,status)) return 1;
    if(autoFCS)
        if (!(*(i-1) & 0x80)==0x80 )  return 2;  //fcs incorrect
    return 0;
}

void Cc2520::flushTxFifoBuffer() const
{
    //reset RX exception
    writeReg(CC2520_EXCFLAG0,CC2520_EXC_TX_FRM_DONE |
                             CC2520_EXC_TX_ACK_DONE |  
                             CC2520_EXC_TX_UNDERFLOW |
                             CC2520_EXC_TX_OVERFLOW); 
    commandStrobe(CC2520_INS_SFLUSHTX);
}

void Cc2520::flushRxFifoBuffer() const
{
    //reset RX exception
    writeReg(CC2520_EXCFLAG0,CC2520_EXC_RX_OVERFLOW);
    writeReg(CC2520_EXCFLAG1,CC2520_EXC_RX_FRM_DONE |
                             CC2520_EXC_RX_FRM_ACCEPETED |
                             CC2520_EXC_SR_MATCH_DONE |
                             CC2520_EXC_SR_MATCH_FOUND |
                             CC2520_EXC_FIFOP |
                             CC2520_EXC_SFD);
    writeReg(CC2520_EXCFLAG2,CC2520_EXC_RX_FRM_ABORTED |
                             CC2520_EXC_RXBUFMOV_TIMEOUT);
    
    commandStrobe(CC2520_INS_SFLUSHRX);
    
    
}

bool Cc2520::sendTxFifoFrame() const
{
    return !isExcRaised(CC2520_EXC_RX_FRM_ABORTED, commandStrobe(CC2520_INS_STXON));
}

bool Cc2520::isTxFrameDone() const
{
    return isExcRaised(CC2520_EXC_TX_FRM_DONE);
}

bool Cc2520::isRxFrameDone() const {
    if (cc2520::fifo_irq::value() == 0 && cc2520::fifop_irq::value() == 1) {
        #ifdef DEBUG_CC2520
            commandStrobe(CC2520_INS_SFLUSHRX);
            unsigned char f0;
            unsigned char f1;
            unsigned char f2;
            readReg(CC2520_EXCFLAG0, f0);
            readReg(CC2520_EXCFLAG1, f1);
            readReg(CC2520_EXCFLAG2, f2);
            printf("Buffer overflow!\n    "
                    "EXCFLAG0: %x\n     "
                    "EXCFLAG1: %x\n    "
                    "EXCFLAG2: %x\n",f0,f1,f2);
            
        #endif //DEBUG_CC2520
        flushRxFifoBuffer();
        return false;
    }
    return cc2520::fifop_irq::value() == 1 ? true : false;
}

bool Cc2520::isRxBufferNotEmpty() const
{
    if(cc2520::fifo_irq::value()==0 && cc2520::fifop_irq::value()==1)
    {
        #ifdef DEBUG_CC2520
           commandStrobe(CC2520_INS_SFLUSHRX);
           printf("Buffer overflow!\n");
           unsigned char f0;
            unsigned char f1;
            unsigned char f2;
            readReg(CC2520_EXCFLAG0, f0);
            readReg(CC2520_EXCFLAG1, f1);
            readReg(CC2520_EXCFLAG2, f2);
            printf("Buffer overflow!\n    "
                    "EXCFLAG0: %x\n     "
                    "EXCFLAG1: %x\n    "
                    "EXCFLAG2: %x\n",f0,f1,f2);
        #endif //DEBUG_CC2520
        flushRxFifoBuffer();
        return false;
    }else if(cc2520::fifo_irq::value()==1 || cc2520::fifop_irq::value()==1)
    {
        return true;
    }
    return false;
}
Cc2520::Mode Cc2520::getMode() const
{
    return this->mode;
}

void Cc2520::setAutoFCS(bool fcs)
{
    autoFCS = fcs;
    if(this->mode != SLEEP || this->mode != DEEP_SLEEP)
            writeReg(CC2520_FRMCTRL0,0x40*fcs);
}


bool Cc2520::writeReg(Cc2520FREG reg, unsigned char data) const
{
    if(reg>0x3F) return false;
    cc2520::cs::low();
    delayUs(1);
    cc2520SpiSendRecv(CC2520_INS_REGISTER_WRITE | reg);
    cc2520SpiSendRecv(data);
    cc2520::cs::high();
    delayUs(1);
    return true;  
}


bool Cc2520::writeReg(Cc2520SREG reg, unsigned char data) const
{
    if(reg>0x7E) return false;
    cc2520::cs::low();
    delayUs(1);
    cc2520SpiSendRecv(CC2520_INS_MEMORY_WRITE);
    cc2520SpiSendRecv(reg);
    cc2520SpiSendRecv(data);
    cc2520::cs::high();
    delayUs(1);
    return true;
}

bool Cc2520::readReg(Cc2520FREG reg, unsigned char& result) const
{
    if(reg>0x3F) return false;
    cc2520::cs::low();
    delayUs(1);
    cc2520SpiSendRecv(CC2520_INS_REGISTER_READ | reg);
    result =cc2520SpiSendRecv();
    cc2520::cs::high();
    delayUs(1);
    return true;
}

 bool Cc2520::readReg(Cc2520SREG reg, unsigned char& result) const
{
    if(reg>0x7E) return false;
    cc2520::cs::low();
    delayUs(1);
    cc2520SpiSendRecv(CC2520_INS_MEMORY_READ);
    cc2520SpiSendRecv(reg);
    result =cc2520SpiSendRecv();
    cc2520::cs::high();
    delayUs(1);
    return true;
}



unsigned char Cc2520::readStatus() const
{
    cc2520::cs::low();
    delayUs(1);
    unsigned char result=cc2520SpiSendRecv(CC2520_INS_SNOP); //NOP command
    cc2520::cs::high();
    delayUs(1);
    return result;
}

unsigned char Cc2520::commandStrobe(Cc2520SpiIsa ins) const
{
    unsigned char status;
    cc2520::cs::low();
    delayUs(1);
    status = cc2520SpiSendRecv(ins);
    cc2520::cs::high();
    delayUs(1);
    return status;
}

bool Cc2520::isExcRaised(Cc2520Exc0 exc, unsigned char status) const
{
    if((status & CC2520_STATUS_EXC_A) == CC2520_STATUS_EXC_A ||
            ((status & CC2520_STATUS_EXC_B) == CC2520_STATUS_EXC_B))
    {
        unsigned char result;
        cc2520::cs::low();
        delayUs(1);
        cc2520SpiSendRecv(CC2520_INS_REGISTER_READ | CC2520_EXCFLAG0);
        result =cc2520SpiSendRecv();
        cc2520::cs::high();
        delayUs(1);
        
        if((result & exc )== exc) 
        {
            #ifdef DEBUG_CC2520
                printf("Exception EXCFLAG0: %x\n",exc);
            #endif //DEBUG_CC2520
            //clear exception
            //bit to 1 will not result in a register change
            cc2520::cs::low();
            delayUs(1);
            cc2520SpiSendRecv(CC2520_INS_REGISTER_WRITE | CC2520_EXCFLAG0);
            cc2520SpiSendRecv(~exc);
            cc2520::cs::high();
            delayUs(1);
            return true;
        }
    }
    return false;
}

bool Cc2520::isExcRaised(Cc2520Exc1 exc, unsigned char status) const
{
    if((status & CC2520_STATUS_EXC_A) == CC2520_STATUS_EXC_A ||
            ((status & CC2520_STATUS_EXC_B) == CC2520_STATUS_EXC_B))
    {
        unsigned char result;
        cc2520::cs::low();
        delayUs(1);
        cc2520SpiSendRecv(CC2520_INS_REGISTER_READ | CC2520_EXCFLAG1);
        result =cc2520SpiSendRecv();
        cc2520::cs::high();
        delayUs(1);
        
        if((result & exc )== exc)  
        {
            #ifdef DEBUG_CC2520
                printf("Exception EXCFLAG1: %x\n",exc);
            #endif //DEBUG_CC2520
            //clear exception
            //bit to 1 will not result in a register change
            cc2520::cs::low();
            delayUs(1);
            cc2520SpiSendRecv(CC2520_INS_REGISTER_WRITE | CC2520_EXCFLAG1);
            cc2520SpiSendRecv(~exc);
            cc2520::cs::high();
            delayUs(1);
            return true;
        }
    }
    return false;
}

bool Cc2520::isExcRaised(Cc2520Exc2 exc, unsigned char status) const
{
    if((status & CC2520_STATUS_EXC_A) == CC2520_STATUS_EXC_A ||
            ((status & CC2520_STATUS_EXC_B) == CC2520_STATUS_EXC_B))
    {
        unsigned char result;
        cc2520::cs::low();
        delayUs(1);
        cc2520SpiSendRecv(CC2520_INS_REGISTER_READ | CC2520_EXCFLAG2);
        result =cc2520SpiSendRecv();
        cc2520::cs::high();
        delayUs(1);
        
        if((result & exc )== exc) 
        {
            #ifdef DEBUG_CC2520
                printf("Exception EXCFLAG2: %x\n",exc);
            #endif //DEBUG_CC2520
            //clear exception
            //bit to 1 will not result in a register change
            cc2520::cs::low();
            delayUs(1);
            cc2520SpiSendRecv(CC2520_INS_REGISTER_WRITE | CC2520_EXCFLAG2);
            cc2520SpiSendRecv(~exc);
            cc2520::cs::high();
            delayUs(1);
            return true;
        }
    }
    return false;
}

inline void Cc2520::initConfigureReg()
{    
    #ifdef DEBUG_CC2520
    printf("Initialize registers.\n");
    #endif //DEBUG_CC2520
    writeReg(CC2520_FREQCTRL,this->frequency-2394); //set frequency
    writeReg(CC2520_FRMCTRL0,0x40*autoFCS); //automatically add FCS
    writeReg(CC2520_FRMCTRL1,0x2); //ignore tx underflow exception
    writeReg(CC2520_TXPOWER,0x32); //TX power 0 dBm
    writeReg(CC2520_FIFOPCTRL,0x7F); //fifop threshold
    
    //No 12 symbol time out after frame reception has ended. (192us)
    writeReg(CC2520_FSMCTRL,0x0);
    
    //10   2 zero simbols
    //0    Lock average level after preamble match
    //0010 4 leading zero bytes of preamble length in TX
    //0    Normal TX filtering
    writeReg(CC2520_MDMCTRL0,0x85); //controls modem
    writeReg(CC2520_MDMCTRL1,0x14); //controls modem
    writeReg(CC2520_FRMFILT0,0x00); //disable frame filtering
    writeReg(CC2520_FRMFILT1,0x00); //disable frame filtering
    writeReg(CC2520_SRCMATCH,0x00); //disable source matching
    
    //Automatically flush tx buffer when an exception buffer overflow take place
    //writeReg(CC2520_EXCBINDX0,0x0A);
    //writeReg(CC2520_EXCBINDX1,0x80 | 0x06 );
    
    //Setting all exception on channel A
    writeReg(CC2520_EXCMASKA0,0XFF);
    writeReg(CC2520_EXCMASKA1,0XFF);
    writeReg(CC2520_EXCMASKA2,0XFF);
    
    writeReg(CC2520_EXCMASKB0,0X00);
    writeReg(CC2520_EXCMASKB1,0X00);
    writeReg(CC2520_EXCMASKB2,0X00);
    
    //Register that need to update from their default value
    writeReg(CC2520_RXCTRL,0x3f);
    writeReg(CC2520_FSCTRL,0x5a);
    writeReg(CC2520_FSCAL1,0x2b);
    writeReg(CC2520_AGCCTRL1,0x11);
    writeReg(CC2520_ADCTEST0,0x10);
    writeReg(CC2520_ADCTEST1,0x0E);
    writeReg(CC2520_ADCTEST2,0x03);
}
