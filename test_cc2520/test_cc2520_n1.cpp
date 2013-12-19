/* 
 * File:   test_cc2520_n1.cpp
 * Author: gigi
 *
 * Created on December 1, 2013, 9:30 AM
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
typedef miosix::Gpio<GPIOB_BASE,0> rx_irq;

/*
 * 
 */
int main(int argc, char** argv) {

    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::INPUT);
    rx_irq::mode(miosix::Mode::INPUT);

    puts("---------------Test cc2520: node1 receiver---------------");
    
    unsigned char packet[5];
    
    Transceiver& cc2520 = Transceiver::instance();
    
    cc2520.setFrequency(2450);
    cc2520.setMode(Transceiver::RX);
    cc2520.setPacketLength(5);
    
    printf("La dimensione del pacchetto è: %d bytes\n",cc2520.getPacketLength());
    
    greenLed::high();
    blueLed::low();
    for(;;)
    {
        while(cc2520.irq()==false);
        while(cc2520.isRxPacketAvailable()) 
        {
            blueLed::high();
            cc2520.readPacket(packet);
            printf("Ho ricevuto: %x\n",*packet);
            blueLed::low(); 
         }
     }
}
