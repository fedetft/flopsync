/* 
 * File:   test_cc2520_n1.cpp
 * Author: gigi
 *
 * Created on December 1, 2013, 9:30 AM
 */

#include <cstdio>
#include <miosix.h>
#include "../drivers/cc2520.h"
#include "../drivers/leds.h"
#include "test_config.h"

using namespace std;
using namespace miosix;

typedef miosix::Gpio<GPIOA_BASE,0> userButton;
typedef miosix::Gpio<GPIOB_BASE,0> rx_irq;

/*
 * 
 */
int main(int argc, char** argv) {

    lowPowerSetup();
    userButton::mode(miosix::Mode::INPUT);
    rx_irq::mode(miosix::Mode::INPUT);

    puts("---------------Test cc2520: node1 receiver---------------");
    
    unsigned char packet[2];
    
    Cc2520& cc2520 = Cc2520::instance();
    
    cc2520.setFrequency(2450);
    cc2520.setMode(Cc2520::RX);
    cc2520.setAutoFCS(true);
    
    led1::high();
    led2::low();
    for(;;)
    {
        //printf("Result isRxFrameDone: %d\n",cc2520.isRxFrameDone());
        if(cc2520.isRxFrameDone()==1)
        {
            led2::high();
            unsigned char len =1;
            printf("readFrame ret: %d\n",cc2520.readFrame(len,packet));
            printf("Ho ricevuto: %x\n",*packet);
            led2::low(); 
        }
     }
}
