/*
 * File:   main.cpp
 * Author: Silvano
 *
 * Created on 2 marzo 2015, 18.53
 *
 */

#include <cstdlib>
#include <cstdio>
#include <miosix.h>
#include <miosix/util/util.h>
#include "drivers/cc2520.h"
#include "drivers/frame.h"
#include "drivers/HW_mapping.h"
#include "drivers/SPI.h"
#include "drivers/BarraLed.h"

using namespace std;
// using namespace miosix;

typedef miosix::Gpio<GPIOC_BASE,8> blueLed;
typedef miosix::Gpio<GPIOC_BASE,9> greenLed;
typedef miosix::Gpio<GPIOB_BASE,0> rx_irq;

typedef miosix::Gpio<GPIOB_BASE,9> trigger_1; //trigger for transceiver 1

/* control pins for transceiver 2 */

typedef miosix::Gpio<GPIOC_BASE,10> reset_2;
typedef miosix::Gpio<GPIOC_BASE,11> vreg_2;     //OK! vreg è appaiabile
typedef miosix::Gpio<GPIOA_BASE,12> trigger_2;
typedef miosix::Gpio<GPIOC_BASE,4> cs_2;

int main(int argc, char** argv)
{
    blueLed::mode(miosix::Mode::OUTPUT);
    rx_irq::mode(miosix::Mode::INPUT);

    cc2520::cs::mode(miosix::Mode::OUTPUT);
    cc2520::cs::high();

    trigger_1::mode(miosix::Mode::OUTPUT);
    trigger_1::low();

    SPI spi;
    Cc2520 cc2520(spi, cc2520::cs::getPin(), cc2520::reset::getPin(), cc2520::vreg::getPin());

    cc2520.setFrequency(2450);
    cc2520.setMode(Cc2520::RX);
    cc2520.setAutoFCS(true); //FIXME: questo abilita il CRC, bisogna mettere false!!!!!!!!!!!
    
    unsigned char *packet=new unsigned char[128];
    unsigned char length;
    
    for(;;)
    {
        if(cc2520.isRxFrameDone() == 1)
        {
            blueLed::high();
            length=64; //FIXME: with 128 fails, because max packet is 127
            cc2520.readFrame(length,packet);
            miosix::memDump(packet,length); //FIXME: la dump modificata si è persa, rifare!!!
            blueLed::low();
            
            switch(length)
            {
                case 4:
                    break;
                case 16:
                {
                    packet16 pkt;
                    memcpy(pkt.getPacket(),packet,pkt.getPacketSize());
                    pair<int, bool> x=pkt.decode();
                    if(x.second) iprintf("Decoded %d\n",x.first);
                    else iprintf("Decode failed\n");
                    break;
                }
                case 64:
                {
                    packet64 pkt;
                    memcpy(pkt.getPacket(),packet,pkt.getPacketSize());
                    pair<int, bool> x=pkt.decode();
                    if(x.second) iprintf("Decoded %d\n",x.first);
                    else iprintf("Decode failed\n");
                    break;
                }  
                case 128:
                {
                    packet128 pkt;
                    memcpy(pkt.getPacket(),packet,pkt.getPacketSize());
                    pair<int, bool> x=pkt.decode();
                    if(x.second) iprintf("Decoded %d\n",x.first);
                    else iprintf("Decode failed\n");
                    break;
                }  
                default:
                    iprintf("Wrong length %d\n",length);
            }
        }
    }
}
