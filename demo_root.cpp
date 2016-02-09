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

#include <iostream>
#include "drivers/cc2520.h"
#include "drivers/timer.h"
#include "drivers/frame.h"
#include "drivers/leds.h"
#include "flopsync_v3/protocol_constants.h"
#include "flopsync_v3/flooder_root_node.h"
#include "flopsync_v3/synchronizer.h"
#include "board_setup.h"
#include "demo.h"

using namespace std;

//HIPEAC demo stuff -- begin
void initAdc()
{
    {
        miosix::FastInterruptDisableLock dLock;
        CMU->HFPERCLKEN0|=CMU_HFPERCLKEN0_ADC0
                        | CMU_HFPERCLKEN0_TIMER0
                        | CMU_HFPERCLKEN0_PRS
                        | CMU_HFPERCLKEN0_USART0;
    }
    
    //HACK: to make usart work here it is necessary to disable it from the bsp
    //and replace the content of Rtc::absoluteSleep() with a call
    //to absoluteWait()
    
    //Set-up USART
    unsigned int baudrate=115200;
    typedef miosix::Gpio<GPIOE_BASE,10> u0tx;
    typedef miosix::Gpio<GPIOE_BASE,11> u0rx;
    u0tx::mode(miosix::Mode::OUTPUT_HIGH);
    u0rx::mode(miosix::Mode::INPUT_PULL_UP_FILTER);
    USART0->IRCTRL=0; //USART0 also has IrDA mode
    USART0->CTRL=USART_CTRL_TXBIL_HALFFULL; //Use the buffer more efficiently
    USART0->FRAME=USART_FRAME_STOPBITS_ONE
            | USART_FRAME_PARITY_NONE
            | USART_FRAME_DATABITS_EIGHT;
    USART0->TRIGCTRL=0;
    USART0->INPUT=0;
    USART0->I2SCTRL=0;
    USART0->ROUTE=USART_ROUTE_LOCATION_LOC0 //Default location
              | USART_ROUTE_TXPEN         //Enable TX pin
              | USART_ROUTE_RXPEN;        //Enable RX pin
    unsigned int periphClock=SystemHFClockGet()/(1<<(CMU->HFPERCLKDIV & 0xf));
    //The number we need is periphClock/baudrate/16-1, but with two bits of
    //fractional part. We divide by 2 instead of 16 to have 3 bit of fractional
    //part. We use the additional fractional bit to add one to round towards
    //the nearest. This gets us a little more precision. Then we subtract 8
    //which is one with three fractional bits. Then we shift to fit the integer
    //part in bits 20:8 and the fractional part in bits 7:6, masking away the
    //third fractional bit. Easy, isn't it? Not quite.
    USART0->CLKDIV=((((periphClock/baudrate/2)+1)-8)<<5) & 0x1fffc0;
    USART0->CMD=USART_CMD_CLEARRX
            | USART_CMD_CLEARTX
            | USART_CMD_TXTRIDIS
            | USART_CMD_RXBLOCKDIS
            | USART_CMD_MASTEREN
            | USART_CMD_TXEN;
//             | USART_CMD_RXEN;
    
    //Set up PRS
    PRS->CH[0].CTRL=PRS_CH_CTRL_ASYNC_DEFAULT
                  | PRS_CH_CTRL_EDSEL_OFF
                  | PRS_CH_CTRL_SOURCESEL_TIMER0
                  | PRS_CH_CTRL_SIGSEL_TIMER0OF;
    
    //Set up timer
    TIMER0->CTRL=TIMER_CTRL_PRESC_DIV1
               | TIMER_CTRL_CLKSEL_PRESCHFPERCLK
               | TIMER_CTRL_MODE_UP;
    TIMER0->TOP=9600-1; //5KHz
               
    
    //Set up ADC
    ADC0->CTRL=ADC_CTRL_OVSRSEL_X4096              //Not used
             | ((48-1)<<16)                        //TIMEBASE 48MHz / 48 =  1MHz
             | ((4-1)<<8)                          //PRESC    48MHz / 4  = 12MHz
             | ADC_CTRL_LPFMODE_RCFILT             //Add some filtering
             | ADC_CTRL_TAILGATE_DEFAULT           //No tailgate
             | ADC_CTRL_WARMUPMODE_KEEPSCANREFWARM;//1us warmup before conversion
    ADC0->SINGLECTRL=ADC_SINGLECTRL_PRSSEL_PRSCH0
                   | ADC_SINGLECTRL_PRSEN          //Trigger conversion on PRS0
                   | ADC_SINGLECTRL_AT_1CYCLE
                   | ADC_SINGLECTRL_REF_1V25
                   | ADC_SINGLECTRL_INPUTSEL_CH4
                   | ADC_SINGLECTRL_RES_12BIT
                   | ADC_SINGLECTRL_ADJ_RIGHT
                   | ADC_SINGLECTRL_DIFF_DEFAULT
                   | ADC_SINGLECTRL_REP_DEFAULT;
    ADC0->IEN=ADC_IEN_SINGLE;
    NVIC_SetPriority(ADC0_IRQn,15);
    NVIC_EnableIRQ(ADC0_IRQn);
    
    //Start timer
    TIMER0->CMD=TIMER_CMD_START;
}

void ADC0_IRQHandler()
{
    ADC0->IFC=ADC_IFC_SINGLE;
    //Unchecked writes, notchecking flags
    unsigned int read=ADC0->SINGLEDATA;
    USART0->TXDATA=read >> 8;
    USART0->TXDATA=read & 0xff;
}
//HIPEAC demo stuff -- end

void readSensor(Timer& timer, Cc2520& transceiver, unsigned long long& when, int node)
{
    static int missed[numNodes]={0};
    
    bool alreadyPrinted=false;
    for(int i=0;i<sendCount;i++)
    {
        DemoPacket packet;
        bool success=getPacketAt(timer,transceiver,&packet,sizeof(packet),
                                 validateByNodeId[node],when,sendFrequencies[i]);
        if(!success) missed[node]++;
        when+=spacing;
        
        if(!success || alreadyPrinted) continue;
        unsigned long long timestamp=timer.getValue();
        cout<<packet.nodeId<<' '
            <<timestamp<<' '
            <<decodeTemperature(packet.thermocoupleTemperature)<<' '
            <<missed[node]<<' '
            <<packet.flopsyncError<<' '
            <<packet.flopsyncMissCount<<endl;
        alreadyPrinted=true;
    }
    if(!alreadyPrinted)
    {
        unsigned long long timestamp=timer.getValue();
        cout<<nodeIds[node]<<' '
            <<timestamp<<' '
            <<"NoPacketFromSensor "
            <<missed[node]<<endl;
    }
}

void readAllSensors(Timer& timer, Cc2520& transceiver, unsigned long long when)
{
    when+=spacing; //First time used for sensing, next used for sending
    for(int i=0;i<numNodes;i++)
        readSensor(timer,transceiver,when,i);
    cout<<"end"<<endl;
}

int main()
{
    lowPowerSetup();
    puts(experimentName);

    //HIPEAC demo stuff -- begin
//     miosix::currentSense::enable::high();
//     initAdc();
    //HIPEAC demo stuff -- end

    Cc2520& transceiver=Cc2520::instance();
    transceiver.setTxPower(Cc2520::P_5);
    #ifndef USE_VHT
    Timer& timer=Rtc::instance();
    #else //USE_VHT
    Timer& timer=VHT::instance();
    #endif //USE_VHT
    FlooderRootNode flooder(timer);
   
    bool removeMe=false;
    for(;;)
    {
        transceiver.setFrequency(2450);
        removeMe^=1;
        if(removeMe) miosix::ledOn(); else miosix::ledOff();
        flooder.synchronize();
        unsigned long long start=flooder.getMeasuredFrameStart();
        for(unsigned long long t=start+initialDelay;t<start+nominalPeriod;t+=dataPeriod)
            readAllSensors(timer,transceiver,t);
    }
}
