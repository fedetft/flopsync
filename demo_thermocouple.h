/***************************************************************************
 *   Copyright (C) 2015 by Terraneo Federico                               *
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

#ifndef DEMO_THERMOCOUPLE
#define DEMO_THERMOCOUPLE

#include <miosix.h>
#include <sstream>

enum TemperatureCode { OK, UNDERFLOW, OVERFLOW, OPENCIRCUIT };

inline short encodeTemperature(short value, TemperatureCode code)
{
    switch(code)
    {
        case OK:          return value;
        case UNDERFLOW:   return -32767;
        case OVERFLOW:    return  32767;
        case OPENCIRCUIT: return -32768;
    }
}

inline std::string decodeTemperature(short value)
{
    switch(value)
    {
        case -32767: return "Underflow";
        case  32767: return "Overflow";
        case -32768: return "OpenCircuit";
    }
    std::ostringstream ss;
    ss<<value;
    return ss.str();
}

using spiSck=miosix::expansion::gpio5;
using spiMiso=miosix::expansion::gpio6;
using spiMosi=miosix::expansion::gpio7;
using th1Cs=miosix::expansion::gpio17;
using th2Cs=miosix::expansion::gpio15;

class ThermocoupleReader
{
public:
    ThermocoupleReader()
    {
        using namespace miosix;
        {
            FastInterruptDisableLock dLock;
            spiSck::mode(Mode::OUTPUT_LOW);
            spiMiso::mode(Mode::INPUT_PULL_DOWN);
            spiMosi::mode(Mode::OUTPUT_LOW);
            th1Cs::mode(Mode::OUTPUT_HIGH);
            th2Cs::mode(Mode::OUTPUT_HIGH);
            CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_USART2;
        }
        delayUs(1);
        //Configuration SPI
        USART2->CTRL=USART_CTRL_MSBF
              | USART_CTRL_SYNC;
        USART2->FRAME=USART_FRAME_STOPBITS_ONE //Should not even be needed
                    | USART_FRAME_PARITY_NONE
                    | USART_FRAME_DATABITS_EIGHT;
        USART2->CLKDIV=((48000000/4000000)-1)<<8; //MAX31855 goes uo tp 5MHz, use 4MHz
        USART2->IEN=0;
        USART2->IRCTRL=0;
        USART2->I2SCTRL=0;
        USART2->ROUTE=USART_ROUTE_LOCATION_LOC0
                    | USART_ROUTE_CLKPEN
                    | USART_ROUTE_TXPEN
                    | USART_ROUTE_RXPEN;
        USART2->CMD=USART_CMD_CLEARRX
                | USART_CMD_CLEARTX
                | USART_CMD_TXTRIDIS
                | USART_CMD_RXBLOCKDIS
                | USART_CMD_MASTEREN
                | USART_CMD_TXEN
                | USART_CMD_RXEN;
        
    }
    
    ~ThermocoupleReader()
    {
        using namespace miosix;
        USART2->CMD=USART_CMD_TXDIS
                  | USART_CMD_RXDIS;
        USART2->ROUTE=0;
        {
            FastInterruptDisableLock dLock;
            spiSck::mode(Mode::DISABLED);
            spiMiso::mode(Mode::DISABLED);
            spiMosi::mode(Mode::DISABLED);
            th1Cs::mode(Mode::DISABLED);
            th2Cs::mode(Mode::DISABLED);
            CMU->HFPERCLKEN0 &= ~CMU_HFPERCLKEN0_USART2;
        }
    }
    
    short readTemperature(int channel)
    {
        int result=0;
        switch(channel)
        {
            case 1: th1Cs::low(); break;
            case 2: th2Cs::low(); break;
        }
        for(int i=0;i<4;i++) { result<<=8; result |= spiSendRecv(); }
        switch(channel)
        {
            case 1: th1Cs::high(); break;
            case 2: th2Cs::high(); break;
        }
        if(result & (1<<16)) return -32768; //FIXME: for now return Open circuit
        return result>>18;
    }
    
private:
    static unsigned char spiSendRecv(unsigned char data = 0)
    {
        USART2->TXDATA=data;
        while((USART2->STATUS & USART_STATUS_RXDATAV)==0) ;
        return USART2->RXDATA;
    }
};

#endif //DEMO_THERMOCOUPLE
