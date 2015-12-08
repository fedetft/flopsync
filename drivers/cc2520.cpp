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
#include "timer.h"
#include <miosix.h>
#include <cstring>
#include "../test_cc2520/test_config.h"
#include <../miosix/kernel/scheduler/scheduler.h>

using namespace miosix;

static Thread *waiting=0;
static volatile bool xoscInterrupt=false;

#ifdef _BOARD_STM32VLDISCOVERY

//Can't simply declare this, as it's also used in the timer
///**
// * EXTI9_5 is connected to the so(PA6) of spi pin, this for irq xosc start
// */
//void __attribute__((naked)) EXTI9_5_IRQHandler()
//{
//	saveContext();
//	asm volatile("bl _Z18xoscIrqhandlerImplv");
//	restoreContext();
//}

#elif defined(_BOARD_POLINODE)

void __attribute__((naked)) GPIO_ODD_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z18xoscIrqhandlerImplv");
    restoreContext();
}

#endif

/**
 * xosx actual implementation
 */
void __attribute__((used)) xoscIrqhandlerImpl()
{
     //Disable interrupt, not just pending bit
    #ifdef _BOARD_STM32VLDISCOVERY
    EXTI->PR=EXTI_PR_PR6;
    #elif defined(_BOARD_POLINODE)
    GPIO->IFC = 1<<1;
    #endif

    xoscInterrupt=true;
    if(!waiting) return;
    waiting->IRQwakeup();
	  if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		Scheduler::IRQfindNextThread();
    waiting=0;
}


//
// class Cc2520
//

Cc2520& Cc2520::instance()
{
    static  Cc2520 singleton;
    return singleton;
}

Cc2520::Cc2520() : autoFCS(true), power(P_0), mode(DEEP_SLEEP), timer(Rtc::instance())
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
                    cc2520::cs::low();
                    //wait until SO=1 (clock stable and running)
                    wait();
                    cc2520::cs::high();

                    commandStrobe(CC2520_INS_SFLUSHTX); //flush TX FIFO
                    break;
                case DEEP_SLEEP:
                    //reset device whit RESETn that automatically start crystal osc.
                    cc2520::reset::low(); //take low for >= 0.1ms     
                    cc2520::vreg::high();
                    timer.wait(static_cast<unsigned long long> (0.0001f*rtcFreq+0.5f));
                    #if WANDSTEM_HW_REV>10
                    flash::cs::high();
                    #endif
                    cc2520::reset::high();
                    cc2520::cs::low();
                    //wait until SO=1 (clock stable and running)
                    wait();
                    cc2520::cs::high();
                    initConfigureReg();
                    commandStrobe(CC2520_INS_SFLUSHTX); //flush TX FIFO         
                    #if CC2520_DEBUG>0
                    probe_xosc_radio_boot::high();
                    probe_xosc_radio_boot::low();
                    #endif//CC2520_DEBUG
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
                    status = commandStrobe(CC2520_INS_SXOSCON); //turn on crystal oscillator
                    isExcRaised(CC2520_EXC_OPERAND_ERROR, status);
                    cc2520::cs::low();
                    //wait until SO=1 (clock stable and running)
                    wait();
                    cc2520::cs::high();
                    status = commandStrobe(CC2520_INS_SFLUSHRX); //flush RX FIFO
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    status = commandStrobe(CC2520_INS_SRXON); //Receive mode
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    break;
                case DEEP_SLEEP:
                    //reset device whit RESETn that automatically start crystal osc.
                    cc2520::reset::low(); //take low for 0.2ms     
                    cc2520::vreg::high();
                    timer.wait(static_cast<unsigned long long> (0.0001f*rtcFreq+0.5f));
                    #if WANDSTEM_HW_REV>10
                    flash::cs::high();
                    #endif
                    cc2520::reset::high();
                    cc2520::cs::low();
                    //wait until SO=1 (clock stable and running)
                    wait();
                    cc2520::cs::high();
                    initConfigureReg();
                    status = commandStrobe(CC2520_INS_SFLUSHRX); //flush RX FIFO
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    status = commandStrobe(CC2520_INS_SRXON); //Receive mode
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    #if CC2520_DEBUG>0
                    probe_xosc_radio_boot::high();
                    probe_xosc_radio_boot::low();
                    #endif//CC2520_DEBUG
                    break;
                case IDLE:
                    status = commandStrobe(CC2520_INS_SFLUSHRX); //flush RX FIFO
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    status = commandStrobe(CC2520_INS_SRXON); //Receive mode
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    #if CC2520_DEBUG>0
                    probe_xosc_radio_boot::high();
                    probe_xosc_radio_boot::low();
                    #endif//CC2520_DEBUG
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
            switch(this->mode)
            {
                case TX:
                case RX:
                case SLEEP:                  
                case IDLE:
                    //reset device will remove side effect 
                    #if WANDSTEM_HW_REV>10
                    flash::cs::low();
                    #endif
                    cc2520::reset::low(); //take low for 0.1ms
                    timer.wait(static_cast<unsigned long long> (0.0001f*rtcFreq+0.5f));
                    cc2520::cs::high();        
                    cc2520::vreg::low();
                    cc2520::reset::high();
                    break; 
               case DEEP_SLEEP:
                    break;
            }
            break;
        case IDLE: 
            switch(this->mode)
            {
                case TX:
                case RX:
                    status = commandStrobe(CC2520_INS_SRFOFF); //radio in IDLE
                    isExcRaised(CC2520_EXC_USAGE_ERROR, status);
                    isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
                    break;
                case SLEEP:
                    commandStrobe(CC2520_INS_SXOSCON);
                    cc2520::cs::low();
                    //wait until SO=1 (clock stable and running)
                    wait();
                    cc2520::cs::high();
                case DEEP_SLEEP :
                    //reset device whit RESETn that automatically start crystal osc.
                    cc2520::reset::low(); //take low for 0.1ms     
                    cc2520::vreg::high();
                    timer.wait(static_cast<unsigned long long> (0.0001f*rtcFreq+0.5f));
                    #if WANDSTEM_HW_REV>10
                    flash::cs::high();
                    #endif
                    cc2520::reset::high();
                    cc2520::cs::low();
                    //wait until SO=1 (clock stable and running)
                    wait();
                    cc2520::cs::high();
                    initConfigureReg();
                    #if CC2520_DEBUG>0
                    probe_xosc_radio_boot::high();
                    probe_xosc_radio_boot::low();
                    #endif//CC2520_DEBUG
                    break;
                case IDLE:
                    break;
            }
            break;  
    }
    this->mode=mode;
    #if CC2520_DEBUG >1
    switch (this->mode)
    {
        case 0: printf("--CC2520_DEBUG-- Setting mode: TX\n");
            break;
        case 1: printf("--CC2520_DEBUG-- Setting mode: RX \n");
            break;
        case 2: printf("--CC2520_DEBUG-- Setting mode: SLEEP\n");
            break;
        case 3: printf("--CC2520_DEBUG-- Setting mode: IDLE\n");
            break;
        case 4: printf("--CC2520_DEBUG-- Setting mode: DEEP_SLEEP\n");
            break;
    }
    #endif //CC2520_DEBUG
    return;
}

void Cc2520::setTxPower(Power power)
{
    this->power=power;
    Mode m=this->mode;           //Temporary needed setMode() changes this->mode
    if(m==DEEP_SLEEP) return;    //At wakeup new power will be set, do nothing
    if(m==SLEEP) setMode(IDLE);  //Need to get out of sleep to write reg
    writeReg(CC2520_TXPOWER,power);
    if(m==SLEEP) setMode(SLEEP); //Get back sleeping if we were sleeping
}
    
void Cc2520::setFrequency(unsigned short freq)
{
    if(freq<2394 || freq>2507) return;
    this->frequency = freq;
    if(this->mode != SLEEP && this->mode != DEEP_SLEEP) 
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
    if(this->mode != SLEEP && this->mode != DEEP_SLEEP){
    cc2520::cs::low();
    delay();
    
    //little endian
    cc2520SpiSendRecv(CC2520_INS_MEMORY_WRITE | 0x3);
    cc2520SpiSendRecv(0xf4);
    cc2520SpiSendRecv(address[1]);
    cc2520SpiSendRecv(CC2520_INS_MEMORY_WRITE | 0x3);
    cc2520SpiSendRecv(0xf5);
    cc2520SpiSendRecv(address[0]);
    
    cc2520::cs::high();
    delay();
    }
}

void Cc2520::setPanId(const unsigned char panId[])
{
    memcpy(this->panId,panId,2);
    if (this->mode != SLEEP  && this->mode != DEEP_SLEEP) 
    {
        cc2520::cs::low();
        delay();

        //little endian
        cc2520SpiSendRecv(CC2520_INS_MEMORY_WRITE | 0x3);
        cc2520SpiSendRecv(0xf2);
        cc2520SpiSendRecv(panId[1]);
        cc2520SpiSendRecv(CC2520_INS_MEMORY_WRITE | 0x3);
        cc2520SpiSendRecv(0xf3);
        cc2520SpiSendRecv(panId[0]);

        cc2520::cs::high();
        delay();
    }
}

int Cc2520::writeFrame(unsigned char length, const unsigned char* pframe)
{
    if(this->mode != TX) return -3;
    if(pframe==NULL) return -2;
    if (length<1 || length>127 ) return -1; 
    cc2520::cs::low();
    delay();
    unsigned char status = cc2520SpiSendRecv(CC2520_INS_TXBUF);
    cc2520SpiSendRecv(length+autoFCS*SIZE_AUTO_FCS); //adding 2 bytes of FCS
    for(int i=0; i<length; i++) cc2520SpiSendRecv(pframe[i]);
    cc2520::cs::high();
    delay();
    if(isExcRaised(CC2520_EXC_TX_OVERFLOW,status)) return 1;
    return 0;
}

int Cc2520::writeFrame(const Frame& frame)
{
    if(this->mode != TX) return -3;
    if(frame.isNotInit()) return -1; //invalid parameter
    cc2520::cs::low();
    delay();
    unsigned char status = cc2520SpiSendRecv(CC2520_INS_TXBUF);
    Frame::ConstIterator i; 
    Frame::ConstIterator end = frame.end();
    #if CC2520_DEBUG >1
        Frame::ConstIterator b=frame.begin(); 
        Frame::ConstIterator e=frame.end();
        printf("--CC2520_DEBUG-- Frame LEN: %d\n",e-b);    
    #endif //CC2520_DEBUG
    for(i=frame.begin(); i<end; i++) {
        cc2520SpiSendRecv(*i);
        #if CC2520_DEBUG >1
            printf("--CC2520_DEBUG-- Byte to send: %x \n",*i);  
        #endif //CC2520_DEBUG
    }
    cc2520::cs::high();
    delay();
    if(isExcRaised(CC2520_EXC_TX_OVERFLOW,status)) return 1; //exc tx overflow
    return 0; //ok
}

int Cc2520::readFrame(unsigned char& length, unsigned char* pframe) const
{
    if(this->mode != RX) return -3;
    short int result = 0;
    if(pframe==NULL) return -2;
    if (length<1 || length>127 ) return -1; 
    unsigned char currLen;
    cc2520::cs::low();
    delay();
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
    unsigned char fcs =0;
    if(autoFCS)
    {
        fcs=cc2520SpiSendRecv();
        #if CC2520_DEBUG >1
            printf("--CC2520_DEBUG-- First byte RSSI: %ddBm\n",((char)fcs)-76);
        #endif //CC2520_DEBUG
        fcs=cc2520SpiSendRecv();
        #if CC2520_DEBUG >1
            printf("--CC2520_DEBUG-- Second byte CRC+Correlation value: %x\n",fcs);
        #endif //CC2520_DEBUG
    }
    cc2520::cs::high();
    delay();
    isExcRaised(CC2520_EXC_RX_FRM_DONE);  //clear FRM_DONE exc
    if(isExcRaised(CC2520_EXC_RX_UNDERFLOW,status)) return 2;
    if(autoFCS)
        if ((fcs & 0x80)==0x00)  return 3;  //fcs incorrect
    return result; 
}

int Cc2520::readFrame(Frame& frame) const
{
    if(this->mode != RX) return -3;
    if(!frame.isNotInit() || frame.isAutoFCS()!=autoFCS) return -1;
    
    cc2520::cs::low();
    delay();
    
    unsigned char status = cc2520SpiSendRecv(CC2520_INS_RXBUF);
    if(!frame.initFrame(cc2520SpiSendRecv())) 
    {
        cc2520::cs::high();
        delay();
        flushRxFifoBuffer();
        return -1;
    }
    Frame::Iterator i;
    Frame::Iterator end = frame.end();
    #if CC2520_DEBUG >1
        Frame::Iterator b=frame.begin(); 
        Frame::Iterator e=frame.end();
        printf("--CC2520_DEBUG-- Frame LEN: %d\n",e-b);    
    #endif //CC2520_DEBUG
    for(i = frame.begin(); i<end; i++) {
        *i = cc2520SpiSendRecv();
        #if CC2520_DEBUG >1
            printf("--CC2520_DEBUG-- payload: %x \n",*i);  
        #endif //CC2520_DEBUG
    }
    unsigned char fcs=0;
    if(autoFCS)
    {
        fcs=cc2520SpiSendRecv();
        #if CC2520_DEBUG >1
            printf("--CC2520_DEBUG-- First byte RSSI: %ddBm\n",((char)fcs)-76);
        #endif //CC2520_DEBUG
        fcs=cc2520SpiSendRecv();
        #if CC2520_DEBUG >1
            printf("--CC2520_DEBUG-- Second byte fcs: %x\n",fcs);
        #endif //CC2520_DEBUG
    }
    cc2520::cs::high();
    delay();
    isExcRaised(CC2520_EXC_RX_FRM_DONE); //clear RX_FRM_DONE exc
    if(isExcRaised(CC2520_EXC_RX_UNDERFLOW,status)) return 1;
    if(autoFCS)
    {
        if ((fcs & 0x80)==0x00 )  return 2;  //fcs incorrect
    }
    return 0;
}

int Cc2520::flushTxFifoBuffer() const
{
    if(this->mode == SLEEP || this->mode == DEEP_SLEEP ) return -1;
    //reset RX exception
    #if CC2520_DEBUG >1
        printf("--CC2520_DEBUG-- Flush TX FIFO buffer\n");   
    #endif //CC2520_DEBUG
    writeReg(CC2520_EXCFLAG0,CC2520_EXC_TX_FRM_DONE |
                             CC2520_EXC_TX_ACK_DONE |  
                             CC2520_EXC_TX_UNDERFLOW |
                             CC2520_EXC_TX_OVERFLOW); 
    commandStrobe(CC2520_INS_SFLUSHTX);
    return true;
}

int Cc2520::flushRxFifoBuffer() const
{
    if(this->mode == SLEEP || this->mode == DEEP_SLEEP ) return -1;
    //reset RX exception
    #if CC2520_DEBUG >1
        printf("--CC2520_DEBUG-- Flush RX FIFO buffer\n");   
    #endif //CC2520_DEBUG
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
    return true;
}

int Cc2520::sendTxFifoFrame() const
{
    if(this->mode != TX) return -1;
    return !isExcRaised(CC2520_EXC_RX_FRM_ABORTED, commandStrobe(CC2520_INS_STXON));
}

int Cc2520::isSFDRaised() const
{
    if(this->mode != TX && this->mode != RX ) return -1;
    return isExcRaised(CC2520_EXC_SFD);
}


int Cc2520::isTxFrameDone() const
{
    if(this->mode != TX) return -1;
    return isExcRaised(CC2520_EXC_TX_FRM_DONE);
}

int Cc2520::isRxFrameDone() const
{
    if(this->mode != RX) return -1;
    
    if(isExcRaised(CC2520_EXC_RX_OVERFLOW))
    {
        #if CC2520_DEBUG >1
            commandStrobe(CC2520_INS_SFLUSHRX);
            unsigned char f0;
            unsigned char f1;
            unsigned char f2;
            readReg(CC2520_EXCFLAG0, f0);
            readReg(CC2520_EXCFLAG1, f1);
            readReg(CC2520_EXCFLAG2, f2);
            printf("--CC2520_DEBUG-- ---------------------------Buffer overflow!\n    "
                    "EXCFLAG0: %x\n     "
                    "EXCFLAG1: %x\n    "
                    "EXCFLAG2: %x\n",f0,f1,f2);
            
        #endif //CC2520_DEBUG
        flushRxFifoBuffer();
        return false;
    }
    return isExcRaised(CC2520_EXC_RX_FRM_DONE);
}

int Cc2520::isRxBufferNotEmpty() const
{
    if(this->mode != RX) return -1;
    if(cc2520::fifo_irq::value()==0 && cc2520::fifop_irq::value()==1)
    {
        #if CC2520_DEBUG >1
           commandStrobe(CC2520_INS_SFLUSHRX);
           printf("--CC2520_DEBUG-- Buffer overflow!\n");
           unsigned char f0;
            unsigned char f1;
            unsigned char f2;
            readReg(CC2520_EXCFLAG0, f0);
            readReg(CC2520_EXCFLAG1, f1);
            readReg(CC2520_EXCFLAG2, f2);
            printf("--CC2520_DEBUG-- Buffer overflow!\n    "
                    "EXCFLAG0: %x\n     "
                    "EXCFLAG1: %x\n    "
                    "EXCFLAG2: %x\n",f0,f1,f2);
        #endif //CC2520_DEBUG
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
    if(this->mode != SLEEP && this->mode != DEEP_SLEEP)
            writeReg(CC2520_FRMCTRL0,0x40*fcs);
}

int Cc2520::stxcal()
{
    if(this->mode != TX && this->mode != RX) return -1;
    //unsigned char status=commandStrobe(CC2520_INS_STXCAL);
    //timer.wait(static_cast<unsigned long long> (0.0002f*vhtFreq+0.5f));
    //return !isExcRaised(CC2520_EXC_RX_FRM_ABORTED, status);
    commandStrobe(CC2520_INS_STXCAL);
    return 1;
}

bool Cc2520::writeReg(Cc2520FREG reg, unsigned char data) const
{
    if(reg>0x3F) return false;
    cc2520::cs::low();
    delay();
    cc2520SpiSendRecv(CC2520_INS_REGISTER_WRITE | reg);
    cc2520SpiSendRecv(data);
    cc2520::cs::high();
    delay();
    return true;  
}


bool Cc2520::writeReg(Cc2520SREG reg, unsigned char data) const
{
    if(reg>0x7E) return false;
    cc2520::cs::low();
    delay();
    cc2520SpiSendRecv(CC2520_INS_MEMORY_WRITE);
    cc2520SpiSendRecv(reg);
    cc2520SpiSendRecv(data);
    cc2520::cs::high();
    delay();
    return true;
}

bool Cc2520::readReg(Cc2520FREG reg, unsigned char& result) const
{
    if(reg>0x3F) return false;
    cc2520::cs::low();
    delay();
    cc2520SpiSendRecv(CC2520_INS_REGISTER_READ | reg);
    result =cc2520SpiSendRecv();
    cc2520::cs::high();
    delay();
    return true;
}

 bool Cc2520::readReg(Cc2520SREG reg, unsigned char& result) const
{
    if(reg>0x7E) return false;
    cc2520::cs::low();
    delay();
    cc2520SpiSendRecv(CC2520_INS_MEMORY_READ);
    cc2520SpiSendRecv(reg);
    result =cc2520SpiSendRecv();
    cc2520::cs::high();
    delay();
    return true;
}



unsigned char Cc2520::readStatus() const
{
    cc2520::cs::low();
    delay();
    unsigned char result=cc2520SpiSendRecv(CC2520_INS_SNOP); //NOP command
    cc2520::cs::high();
    delay();
    return result;
}

unsigned char Cc2520::commandStrobe(Cc2520SpiIsa ins) const
{
    unsigned char status;
    cc2520::cs::low();
    delay();
    status = cc2520SpiSendRecv(ins);
    cc2520::cs::high();
    delay();
    return status;
}

bool Cc2520::isExcRaised(Cc2520Exc0 exc, unsigned char status) const
{
    if((status & CC2520_STATUS_EXC_A) == CC2520_STATUS_EXC_A ||
            ((status & CC2520_STATUS_EXC_B) == CC2520_STATUS_EXC_B))
    {
        unsigned char result;
        cc2520::cs::low();
        delay();
        cc2520SpiSendRecv(CC2520_INS_REGISTER_READ | CC2520_EXCFLAG0);
        result =cc2520SpiSendRecv();
        cc2520::cs::high();
        delay();
        
        if((result & exc )== exc) 
        {
            #if CC2520_DEBUG >2
                printf("--CC2520_DEBUG-- Exception EXCFLAG0: %x\n",exc);
            #endif //CC2520_DEBUG
            //clear exception
            //bit to 1 will not result in a register change
            cc2520::cs::low();
            delay();
            cc2520SpiSendRecv(CC2520_INS_REGISTER_WRITE | CC2520_EXCFLAG0);
            cc2520SpiSendRecv(~exc);
            cc2520::cs::high();
            delay();
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
        delay();
        cc2520SpiSendRecv(CC2520_INS_REGISTER_READ | CC2520_EXCFLAG1);
        result =cc2520SpiSendRecv();
        cc2520::cs::high();
        delay();
        
        if((result & exc )== exc)  
        {
            #if CC2520_DEBUG >2
                printf("--CC2520_DEBUG-- Exception EXCFLAG1: %x\n",exc);
            #endif //CC2520_DEBUG
            //clear exception
            //bit to 1 will not result in a register change
            cc2520::cs::low();
            delay();
            cc2520SpiSendRecv(CC2520_INS_REGISTER_WRITE | CC2520_EXCFLAG1);
            cc2520SpiSendRecv(~exc);
            cc2520::cs::high();
            delay();
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
        delay();
        cc2520SpiSendRecv(CC2520_INS_REGISTER_READ | CC2520_EXCFLAG2);
        result =cc2520SpiSendRecv();
        cc2520::cs::high();
        delay();
        
        if((result & exc )== exc) 
        {
            #if CC2520_DEBUG >2
                printf("--CC2520_DEBUG-- Exception EXCFLAG2: %x\n",exc);
            #endif //CC2520_DEBUG
            //clear exception
            //bit to 1 will not result in a register change
            cc2520::cs::low();
            delay();
            cc2520SpiSendRecv(CC2520_INS_REGISTER_WRITE | CC2520_EXCFLAG2);
            cc2520SpiSendRecv(~exc);
            cc2520::cs::high();
            delay();
            return true;
        }
    }
    return false;
}

void Cc2520::initConfigureReg()
{    
    writeReg(CC2520_FREQCTRL,this->frequency-2394); //set frequency
    writeReg(CC2520_FRMCTRL0,0x40*autoFCS); //automatically add FCS
    writeReg(CC2520_FRMCTRL1,0x2); //ignore tx underflow exception
    writeReg(CC2520_FIFOPCTRL,0x7F); //fifop threshold
    
    //No 12 symbol timeout after frame reception has ended. (192us)
    writeReg(CC2520_FSMCTRL,0x0);
    
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
    
    //Setting SFD, TX_FRM_DONE, RX_FRM_DONE on channel B
    writeReg(CC2520_EXCMASKB0,CC2520_EXC_TX_FRM_DONE);
    writeReg(CC2520_EXCMASKB1,CC2520_EXC_RX_FRM_DONE | CC2520_EXC_SFD);
    writeReg(CC2520_EXCMASKB2,0X00);
    
    //Register that need to update from their default value (as recommended datasheet)
    writeReg(CC2520_TXPOWER,power); //TX power
    writeReg(CC2520_CCACTRL0,0xF8);
    writeReg(CC2520_MDMCTRL0,0x85); //controls modem
    writeReg(CC2520_MDMCTRL1,0x14); //controls modem
    writeReg(CC2520_RXCTRL,0x3f);
    writeReg(CC2520_FSCTRL,0x5a);
    writeReg(CC2520_FSCAL1,0x2b);
    writeReg(CC2520_AGCCTRL1,0x11);
    writeReg(CC2520_ADCTEST0,0x10);
    writeReg(CC2520_ADCTEST1,0x0E);
    writeReg(CC2520_ADCTEST2,0x03);
    
    //Setting gpio5 as input on rising edge send command strobe STXON
    writeReg(CC2520_GPIOCTRL5,0x80|0x08);
    //Setting gpio3 as output exception channel B
    writeReg(CC2520_GPIOCTRL3,0x22);
    #if CC2520_DEBUG >2
    printf("--CC2520_DEBUG-- Registers initialized.\n");
    #endif //CC2520_DEBUG
}

void Cc2520::wait()
{
    FastInterruptDisableLock dLock;
    #ifdef _BOARD_STM32VLDISCOVERY
    EXTI->IMR |= EXTI_IMR_MR6;
    EXTI->RTSR |= EXTI_RTSR_TR6;
    EXTI->PR=EXTI_PR_PR6; //Clear eventual pending IRQ
    pinirq saved=IRQreplaceExtiIrq(xoscIrqhandlerImpl);
    #elif defined(_BOARD_POLINODE)
    GPIO->INSENSE |= 1<<0;
    GPIO->EXTIPSELL &= ~(0x7<<4);
    GPIO->EXTIPSELL |= (0x3<<4);
    GPIO->EXTIRISE |= 1<<1;
    GPIO->IFC = 1<<1; //Clear eventual pending IRQ
    GPIO->IEN |= 1<<1;
    #endif

    if(cc2520::miso::value()==0)
    {
        xoscInterrupt=false;
        while(!xoscInterrupt)
        {
            waiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    }
    #ifdef _BOARD_STM32VLDISCOVERY
    IRQreplaceExtiIrq(saved);
    EXTI->IMR &= ~EXTI_IMR_MR6;
    EXTI->RTSR &= ~EXTI_RTSR_TR6;
    #elif defined(_BOARD_POLINODE)
    GPIO->IEN &= ~(1<<1);
    #endif
}