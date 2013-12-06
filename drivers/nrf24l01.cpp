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

#include "nrf24l01.h"
#include "nrf24l01_config.h"
#include "rtc.h"
#include <miosix.h>
#include <cstring>
#include "../test_cc2520/test_config.h"
using namespace miosix;

//
// class Nrf24l01
//

Nrf24l01::Nrf24l01() : frequency(2400), length(16), mode(SLEEP)
{
    nrfGpioInit();
    setMode(SLEEP);
}

void Nrf24l01::setMode(Mode mode)
{
    nrf::ce::low(); //Make sure CE=0   
    writeReg(STATUS, RX_DR | TX_DS); //Clear IRQ flags

    //If the new mode is SLEEP do just that
    if(mode==SLEEP)
    {
        this->mode=mode;
        writeReg(CONFIG,0);
        return;
    }
    
    //If old mode was SLEEP reconfigure the whole thing (just to be safe)
    if(this->mode==SLEEP)
    {
        writeReg(EN_AA,0);     //No enhanced shockburst
        writeReg(EN_RXADDR,1); //Enable only pipe 0
        writeReg(SETUP_AW,1);  //3 byte address
        writeReg(SETUP_RETR,0);//No auto retransmit
        writeReg(RF_CH,frequency-2400); //Set frequency
        writeReg(RF_SETUP,0x6);//1Mbps, 0dBm
        writeReg(FEATURE,0);   //No dynamic packet length or auto ack
        writeReg(RX_ADDR,address,3);
        writeReg(TX_ADDR,address,3);
        writeReg(RX_PW_P0,length); //Set packet size
    }

    if(mode==TX)
    {
        writeReg(CONFIG,0xe); //Transmitter mode
        nrf::cs::low();
        nrfSpiSendRecv(0xe1); //Flush TX fifo
        nrf::cs::high();
        delayUs(1);
    } else {
        writeReg(CONFIG,0xf); //Receiver mode
        nrf::cs::low();
        nrfSpiSendRecv(0xe2); //Flush RX fifo
        nrf::cs::high();
        delayUs(1);
    }
    
    if(this->mode==SLEEP)
    {
        // Wait 1.5ms if old mode was SLEEP
        // Note that the time to send the SPI commands is around 100us resulting
        // in this code taking around 1.6ms
        AuxiliaryTimer& timer=AuxiliaryTimer::instance();
        timer.initTimeoutTimer(static_cast<int>(0.0015f*16384+0.5f));
        timer.waitForTimeout();
    }
    
//    if(mode==RX) nrf::ce::high(); //Start receiving right now if RX
    this->mode=mode;
}

void Nrf24l01::setFrequency(unsigned short freq)
{
    if(freq<2400 || freq>2500) return;
    this->frequency=freq;
    if(this->mode!=SLEEP) writeReg(RF_CH,freq-2400);
}
    
void Nrf24l01::setPacketLength(unsigned short length)
{
    if(length<1 || length>32) return;
    this->length=length;
    if(this->mode!=SLEEP) writeReg(RX_PW_P0,length); //Set packet size
}

void Nrf24l01::setAddress(const unsigned char address[3])
{
    memcpy(this->address,address,3);
    if(this->mode!=SLEEP)
    {
        writeReg(RX_ADDR,address,3);
        writeReg(TX_ADDR,address,3);
    }
}

bool Nrf24l01::irq() const
{
    return nrf::irq::value()==0 ? true : false;
}

void Nrf24l01::writePacket(const void *packet)
{
    const unsigned char *data=reinterpret_cast<const unsigned char*>(packet);
    nrf::cs::low();
    nrfSpiSendRecv(0xa0);
    for(int i=0;i<this->length;i++) nrfSpiSendRecv(*data++);
    nrf::cs::high();
    nrf::ce::high();
    delayUs(1);
}

bool Nrf24l01::isTxSlotAvailable() const
{
    return (readStatus() & 0x1) != 0x1;
}

bool Nrf24l01::allPacketsSent() const
{
    nrf::cs::low();
    nrfSpiSendRecv(FIFO_STATUS);
    bool result=(nrfSpiSendRecv() & 0x10) == 0x10;
    nrf::cs::high();
    delayUs(1);
    return result;
}

void Nrf24l01::clearTxIrq()
{
    writeReg(STATUS,TX_DS); //Clear TX_DS interrupt flag in the nrf chip
}

void Nrf24l01::endWritePacket()
{
    nrf::ce::low();
    writeReg(STATUS,TX_DS); //Clear TX_DS interrupt flag in the nrf chip
}

void Nrf24l01::startReceiving()
{
    if(mode==Transceiver::RX) nrf::ce::high(); //Start receiving right now if RX
}

void Nrf24l01::readPacket(void *packet)
{
    unsigned char *data=reinterpret_cast<unsigned char*>(packet);
    nrf::cs::low();
    nrfSpiSendRecv(0x61);
    for(int i=0;i<this->length;i++) *data++=nrfSpiSendRecv();
    nrf::cs::high();
    delayUs(1);
    writeReg(STATUS,RX_DR); //Clear RX_DR interrupt flag in the nrf chip
}

bool Nrf24l01::isRxPacketAvailable() const
{
    return (readStatus() & 0xe) != 0xe;
}

void Nrf24l01::writeReg(NrfReg reg, unsigned char data)
{
    if(reg>31) return;
    nrf::cs::low();
    nrfSpiSendRecv(0x20 | reg);
    nrfSpiSendRecv(data);
    nrf::cs::high();
    delayUs(1);
}

void Nrf24l01::writeReg(NrfReg reg, const unsigned char *data, int len)
{
    if(len<0 || len>5 || reg>31) return;
    nrf::cs::low();
    nrfSpiSendRecv(0x20 | reg);
    for(int i=0;i<len;i++) nrfSpiSendRecv(*data++);
    nrf::cs::high();
    delayUs(1);
}

unsigned char Nrf24l01::readStatus() const
{
    nrf::cs::low();
    unsigned char result=nrfSpiSendRecv(0xff); //NOP command
    nrf::cs::high();
    delayUs(1);
    return result;
}

Nrf24l01::~Nrf24l01(){}