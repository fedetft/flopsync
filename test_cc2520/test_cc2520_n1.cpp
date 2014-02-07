/* 
 * File:   test_cc2520_n1.cpp
 * Author: gigi
 *
 * Created on December 1, 2013, 9:30 AM
 */

#include <cstdio>
#include <miosix.h>
#include "../drivers/cc2520.h"
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
    
    Cc2520& cc2520 = Cc2520::instance();
    
    cc2520.setFrequency(2450);
    cc2520.setMode(Cc2520::RX);
    
    greenLed::high();
    blueLed::low();
    for(;;)
    {
        //printf("Result isRxFrameDone: %d\n",cc2520.isRxFrameDone());
        if(cc2520.isRxFrameDone()==1)
        {
            blueLed::high();
            unsigned char len =1;
            cc2520.readFrame(len,packet);
            printf("Ho ricevuto: %x\n",*packet);
            blueLed::low(); 
        }
     }
}
