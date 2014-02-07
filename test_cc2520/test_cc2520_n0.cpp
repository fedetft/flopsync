/* 
 * File:   test_cc2520_n0.cpp
 * Author: gigi
 *
 * Created on November 29, 2013, 3:45 PM
 */

#include <cstdio>
#include "../drivers/cc2520.h"
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
    Cc2520& cc2520 = Cc2520::instance();
    
    cc2520.setFrequency(2450);
    cc2520.setMode(Cc2520::TX);

    greenLed::high();
    blueLed::low();
    
    for(;;)
    {        
        if (userButton::value()==1)
        {
           //MemoryProfiling::print();
            blueLed::high();
            cont++;
            cc2520.writeFrame(1,pCont);
            cc2520.sendTxFifoFrame();
            while(!cc2520.isTxFrameDone()) printf("TX busy\n"); //Wait
            blueLed::low();
            printf("Ho inviato: %x \n",cont);
        }
        //delayMs(500);
    }
}
