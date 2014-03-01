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
#include "../drivers/timer.h"
#include "../drivers/frame.h"

using namespace std;

typedef miosix::Gpio<GPIOC_BASE,8> blueLed;
typedef miosix::Gpio<GPIOC_BASE,9> greenLed;
typedef miosix::Gpio<GPIOA_BASE,0> userButton;

Frame *pFrame ;

/*
 * 
 */
int main(int argc, char** argv) {

    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::INPUT);

    Timer& timer=VHT::instance();
    
    puts("---------------Test cc2520: node0 transmitter----------------");
   
    //const char* packet="HELLO";
    unsigned char cont = 0x0;
    unsigned char* pCont =&cont;
    Cc2520& cc2520 = Cc2520::instance();
    
    cc2520.setFrequency(2450);
    cc2520.setMode(Cc2520::TX);
    cc2520.setAutoFCS(true);
    
    greenLed::high();
    blueLed::low();
    
//    for(;;)
//    {        
//        if (userButton::value()==1)
//        {
//           //MemoryProfiling::print();
//            blueLed::high();
//            cont++;
//            cc2520.writeFrame(1,pCont);
//            timer.wait(1*vhtFreq);
//            pin15::high();
//            pin15::low();
//            while(!cc2520.isTxFrameDone()) printf("TX busy\n"); //Wait
//            cc2520.isSFDRaised();
//            blueLed::low();
//            printf("Ho inviato: %x \n",cont);
//        }
//        //delayMs(500);
//    }
    pFrame = new Frame(1,false,true);
    unsigned int i=1;
    unsigned char packet[2];
    for(;;)
    {        
       //MemoryProfiling::print();
        cont++;
        pFrame->setPayload(pCont);
        printf("writeFrame ret: %d\n",cc2520.writeFrame(*pFrame));
        //packet[0]=cont;
        //packet[1]=cont;
        //cc2520.writeFrame(2,packet);
        cc2520.stxcal();
        timer.absoluteWaitTrigger(1*vhtFreq*i);
        blueLed::high();
        i++;
        while(!cc2520.isTxFrameDone()) printf("TX busy\n"); //Wait
        cc2520.isSFDRaised();
        blueLed::low();
        printf("Ho inviato: %x \n",cont);
     }
}
