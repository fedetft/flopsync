/* 
 * File:   test_cc2520_n0.cpp
 * Author: gigi
 *
 * Created on November 29, 2013, 3:45 PM
 */

#include <cstdio>
#include "../drivers/transceiver.h"
#include <miosix.h>
#include "test_config.h"

using namespace std;
using namespace miosix;

typedef miosix::Gpio<GPIOC_BASE,8> blueLed;
typedef miosix::Gpio<GPIOC_BASE,9> greenLed;
typedef miosix::Gpio<GPIOA_BASE,0> userButton;

/*
 * 
 */
int main(int argc, char** argv) {

    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::INPUT);

    puts("---------------Test cc2520: node0 transmitter----------------");
   
    //const char* packet="HELLO";
    unsigned char cont = 0x0;
    unsigned char* pCont =&cont;
    Transceiver& cc2520 = Transceiver::instance();
    
    cc2520.setFrequency(2450);
    cc2520.setMode(Transceiver::TX);
    cc2520.setPacketLength(1);
    
    printf("La dimensione del pacchetto è: %d bytes\n",cc2520.getPacketLength());

    greenLed::high();
    blueLed::low();
    
    for(;;)
    {        
        if (userButton::value()==1)
        {
           //MemoryProfiling::print();
            blueLed::high();
            cont++;
            cc2520.writePacket(pCont);
            while(cc2520.irq()==false) printf("TX busy\n"); //Wait
            cc2520.endWritePacket();
            blueLed::low();
            printf("Ho inviato: %x \n",cont);
        }
        //delayMs(500);
    }
}
