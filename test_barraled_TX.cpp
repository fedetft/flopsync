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
#include "gh.h"
#include "fast_trigger.h"

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
typedef miosix::Gpio<GPIOC_BASE,5> cs_2;

/* control pins for transceiver 3 */

typedef miosix::Gpio<GPIOC_BASE,0> reset_3;
typedef miosix::Gpio<GPIOC_BASE,1> vreg_3;   
typedef miosix::Gpio<GPIOC_BASE,2> trigger_3;
typedef miosix::Gpio<GPIOC_BASE,3> cs_3;


template<typename T>
void sendBarraLed(Cc2520& cc2520_1, Cc2520& cc2520_2, Cc2520& cc2520_3,
        unsigned char id, int skew, int power)
{
    iprintf("skew=%d power=%d\n",skew,power);
    for(int i=0;i<4;i++)
    {
        const unsigned char pkt[]={id,id,id,id};
        cc2520_1.writeFrame(sizeof(pkt),pkt);
        miosix::memDump(pkt,sizeof(pkt));
        trigger_1::high();
        miosix::Thread::sleep(1);
        trigger_1::low();
        while(!cc2520_1.isTxFrameDone()) ;
        cc2520_1.isSFDRaised();
        miosix::Thread::sleep(500);
    }
    
    T *pkt_1=new T;
    T *pkt_2=new T;
    T *pkt_i=new T;
    pkt_1->encode(pkt_1->getPacketSize()-2); //The packets differ by 4 nibbles
    pkt_2->encode(pkt_2->getPacketSize()+2);
    pkt_i->encode(pkt_i->getPacketSize()/2);
    
    for(int i=0;i<40;i++)
    {
        greenLed::high();
        
        switch(power)
        {
            case 1:
                cc2520_3.setTxPower(Cc2520::P_m18);
                break;
            case 2:
                cc2520_3.setTxPower(Cc2520::P_m7);
                break; 
            case 3:
                cc2520_3.setTxPower(Cc2520::P_m2);
                break;
        }

        cc2520_1.writeFrame(pkt_1->getPacketSize(),pkt_1->getPacket());
        cc2520_2.writeFrame(pkt_2->getPacketSize(),pkt_2->getPacket());
        if(power>0) cc2520_3.writeFrame(pkt_i->getPacketSize(),pkt_i->getPacket());

        miosix::memDump(pkt_1->getPacket(), pkt_1->getPacketSize());
        miosix::memDump(pkt_2->getPacket(), pkt_2->getPacketSize());
        if(power>0) miosix::memDump(pkt_i->getPacket(), pkt_i->getPacketSize());
        
        // skew : time skew in ns
        //    0 : 83ns
        //    1 : 166ns
        //    2 : 333ns
        //    3 : 624ns
        if(skew == 0)
        {
            miosix::FastInterruptDisableLock lock;
//             trigger_1::high();
//             trigger_2::high();
	    trigger_80ns();
            if(power>0) trigger_3::high();
        } else {
            miosix::FastInterruptDisableLock lock;
//             trigger_1::high();
//             for(int k=0;k<skew;k++) asm volatile("nop");
//             trigger_2::high();
	    
        switch(skew){
	    
        case 1:
            trigger_160ns();
            break;
	      
        case 2:
            trigger_320ns();
            break;

        case 3:
            trigger_640ns();
            break;
	      
	    }
	    
            if(power>0) trigger_3::high();
        }

        miosix::Thread::sleep(1);
        
        trigger_1::low();
        trigger_2::low();
        if(power>0) trigger_3::low();

        if(power>0)
        {
            while(!cc2520_1.isTxFrameDone() || !cc2520_2.isTxFrameDone()
            || !cc2520_3.isTxFrameDone());
        } else {
            while(!cc2520_1.isTxFrameDone() || !cc2520_2.isTxFrameDone());
        }
        cc2520_1.isSFDRaised();
        cc2520_2.isSFDRaised();
        if(power>0) cc2520_3.isSFDRaised();
        
        greenLed::low();
        miosix::Thread::sleep(500);
    }
    delete pkt_1;
    delete pkt_2;
    delete pkt_i;
}

int main(int argc, char** argv)
{
    greenLed::mode(miosix::Mode::OUTPUT);
    blueLed::mode(miosix::Mode::OUTPUT);

    trigger_1::mode(miosix::Mode::OUTPUT);
    trigger_1::low();

    trigger_2::mode(miosix::Mode::OUTPUT);
    trigger_2::low();
    
    trigger_3::mode(miosix::Mode::OUTPUT);
    trigger_3::low();

    SPI spi;

    Cc2520 cc2520_1(spi, cc2520::cs::getPin(), cc2520::reset::getPin(), cc2520::vreg::getPin());
    Cc2520 cc2520_2(spi, cs_2::getPin(), reset_2::getPin(), vreg_2::getPin());
    Cc2520 cc2520_3(spi, cs_3::getPin(), reset_3::getPin(), vreg_3::getPin());
    
    cc2520_1.setFrequency(2450);
    cc2520_1.setMode(Cc2520::TX);
    cc2520_1.setAutoFCS(true); //FIXME: questo abilita il CRC, bisogna mettere false!!!!!!!!!!!
    cc2520_1.setTxPower(Cc2520::P_0); //TODO: was this the default?? check!!
    
    cc2520_2.setFrequency(2450);
    cc2520_2.setMode(Cc2520::TX);
    cc2520_2.setAutoFCS(true); //FIXME: questo abilita il CRC, bisogna mettere false!!!!!!!!!!!
    cc2520_2.setTxPower(Cc2520::P_0);
    
    cc2520_3.setFrequency(2450);
    cc2520_3.setMode(Cc2520::TX);
    cc2520_3.setAutoFCS(true); //FIXME: questo abilita il CRC, bisogna mettere false!!!!!!!!!!!
    cc2520_3.setTxPower(Cc2520::P_0);
    
    for(;;)
    {
        for(int j=0;j<4;j++)
        {
            for(int i=0;i<4;i++)
            {
                //meaning of id parameter to sendBarraLed
                // 00000000
                // |||||+++-- skew
                // ||+++----- power of inteferer
                // ++-------- 0=16 byte packets, 1=64 byte, 2=128 byte
                sendBarraLed<packet16>(cc2520_1,cc2520_2,cc2520_3,0x00 | j<<3 | i,i,j);
                sendBarraLed<packet64>(cc2520_1,cc2520_2,cc2520_3,0x40 | j<<3 | i,i,j);
//                 sendBarraLed<packet128>(cc2520_1,cc2520_2,cc2520_3,0x80 | j<<3 | i,i,j); //FIXME: locks up, why? because max packet is 127, not 128
            }
        }
    }
}