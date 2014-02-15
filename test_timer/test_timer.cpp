/* 
 * File:   main.cpp
 * Author: gigi
 *
 * Created on January 20, 2014, 10:40 AM
 */
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include "../drivers/timer.h"
#include "test_config.h"
#include <miosix.h>

using namespace std;

typedef miosix::Gpio<GPIOC_BASE,8> blueLed;
typedef miosix::Gpio<GPIOC_BASE,9> greenLed;
typedef miosix::Gpio<GPIOA_BASE,0> userButton;

static void inline testRtc()
{
    puts("---------------Test timer RTC----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
    greenLed::high();
    blueLed::high();
    Timer& rtc=Rtc::instance();
    
    int i=1;
    rtc.setValue(0xFFFFFFFFll - 5/2*rtcFreq); 
    printf("Timestamp:  %016llX\n",rtc.getValue());
    for(;;)
    {        
        blueLed::low(); 
        rtc.setAbsoluteWakeupSleep((0xFFFFFFFFll-5/2*rtcFreq) + (i*rtcFreq)); //1 secondo
        rtc.sleep();
        blueLed::high();  
        printf("Timestamp:  %016llX\n",rtc.getValue());
        i++;
    }
}

static void inline testRtcWaitExtEventOrTimeout()
{
    puts("---------------Test timer RTC wait for event or timeout----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
    greenLed::high();
    blueLed::high();
    Timer& rtc=Rtc::instance();
    
    rtc.setValue(0xFFFFFFFFll - 5/2*rtcFreq);
    printf("Timestamp:  %016llX\n",rtc.getValue());
    int i=1;
    bool timeout;
    for(;;)
    {        
        blueLed::low(); 
        rtc.setAbsoluteTimeoutForEvent((0xFFFFFFFFll-5/2*rtcFreq) + (rtcFreq*i)); //un secondo
        timeout=rtc.wait();
        blueLed::high(); 
        timeout?
            printf("Timestamp:  %016llX\n",rtc.getValue()):
            printf("Packet   :  %016llX\n",rtc.getExtEventTimestamp());
        i++;
    }
}

static void inline testRtcTriggerEvent()
{
    puts("---------------Test timer RTC----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::INPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    greenLed::high();
    Timer& rtc=Rtc::instance();
    
    int i=1;
    rtc.setValue(0xFFFFFFFFll - 5/2*rtcFreq); 
    printf("Timestamp:  %016llX\n",rtc.getValue());
    for(;;)
    {        
        rtc.setAbsoluteTriggerEvent((0xFFFFFFFFll-5/2*rtcFreq) + (i*rtcFreq)); //1 secondo
        rtc.wait();
        printf("Timestamp:  %016llX\n",rtc.getValue());
        i++;
    }
}

static void inline testVhtSleep()
{
    puts("---------------Test timer VHT sleep----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
    greenLed::high();
    blueLed::high();
    Timer& vht=VHT::instance();
    
    int i=1;
    vht.setValue(vhtFreq*1/20); //mezzo secondo 
    printf("Timestamp:  %016llX\n",vht.getValue());
    for(;;)
    {    
        blueLed::low(); 
        vht.setAbsoluteWakeupSleep(vhtFreq*1/2+vhtFreq*i); //un secondo
        vht.sleep();
        blueLed::high(); 
        printf("Timestamp:  %016llX\n",vht.getValue());
        i++;
    }
}

static void inline testVhtGetPacketTimestamp()
{
    puts("---------------Test timer VHT get timestamp event----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
    greenLed::high();
    blueLed::low();
    Timer& vht=VHT::instance();
    for(;;)
    {        
        miosix::delayMs(5000);
        printf("Timestamp:  %016llX\n",vht.getExtEventTimestamp());
    }
}


static void inline testVhtWait()
{
    puts("---------------Test timer VHT wait----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
    greenLed::high();
    blueLed::high();
    Timer& vht=VHT::instance();
    
    vht.setValue(vhtFreq*1/20); //mezzo secondo 
    printf("Timestamp:  %016llX\n",vht.getValue());
    int i=1;
    for(;;)
    {        
        blueLed::low();
        vht.setAbsoluteWakeupWait(vhtFreq*1/2+vhtFreq*i); //un secondo
        vht.wait();
        blueLed::high();
        printf("Timestamp:  %016llX\n",vht.getValue());
        i++;
    }
}

static void inline testVhtWaitThick()
{
    puts("---------------Test timer VHT wait thick----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
    greenLed::high();
    blueLed::high();
    Timer& vht=VHT::instance();
    
    vht.setValue(vhtFreq*1/20); //mezzo secondo 
    printf("Timestamp:  %016llX\n",vht.getValue());
    int i=1;
    for(;;)
    {        
        blueLed::low();
        vht.wait(static_cast<unsigned long long>(1*vhtFreq));
        blueLed::high();
        printf("Timestamp:  %016llX\n",vht.getValue());
        i++;
    }
}

static void inline testVhtWaitExtEventOrTimeout()
{
    puts("---------------Test timer VHT wait for event or timeout----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
    greenLed::high();
    blueLed::high();
    Timer& vht=VHT::instance();
    
    vht.setValue(vhtFreq*1/2); //mezzo secondo 
    printf("Timestamp:  %016llX\n",vht.getValue());
    int i=1;
    bool timeout;
    for(;;)
    {        
        blueLed::low(); 
        vht.setAbsoluteTimeoutForEvent(vhtFreq*1/2+vhtFreq*i); //un secondo
        timeout=vht.wait();
        blueLed::high(); 
        timeout?
            printf("Timestamp:  %016llX\n",vht.getValue()):
            printf("Packet   :  %016llX\n",vht.getExtEventTimestamp());
        i++;
    }
}

static void inline testVhtWaitExtEvent()
{
    puts("---------------Test timer wait for event---------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
    greenLed::high();
    blueLed::high();
    Timer& vht=VHT::instance();
    
    vht.setValue(vhtFreq*1/2); //mezzo secondo
    printf("Timestamp:  %016llX\n",vht.getValue());
    bool timeout;
    for(;;)
    {        
        blueLed::low(); 
        vht.setAbsoluteTimeoutForEvent(0); 
        timeout=vht.wait();
        blueLed::high(); 
        timeout?
            printf("Timestamp:  %016llX\n",vht.getValue()):
            printf("Packet   :  %016llX\n",vht.getExtEventTimestamp());
    }
}

static void inline testVhtMonotonic()
{
    puts("---------------Test timer VHT monotonic----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
    unsigned long long prec=0;
    unsigned long long last=0;

    #if TIMER_DEBUG ==4 
    typeTimer precInfo;
    typeTimer lastInfo;
    #endif//TIMER_DEBUG
    greenLed::high();
    blueLed::high();
    VHT& vht=VHT::instance();
    printf("Clock monotonic....\n");
    for(;;)
    {        
        blueLed::low(); 
        prec = last;
        #if TIMER_DEBUG ==4
        //precInfo = lastInfo;
        #endif//TIMER_DEBUG
        vht.setAbsoluteTimeoutForEvent(0); 
        vht.wait();
        blueLed::high(); 
        last = vht.getExtEventTimestamp();
        #if TIMER_DEBUG < 4
        assert(last>prec);
        #else//TIMER_DEBUG
        lastInfo = vht.getInfo();
        if(last<=prec)
            printf("No monotonic clock:\n" ""
                   "prec=%016llX  ts=%016llX  ovh=%016llX  cnt=%X  sr=%X cnt=%X  sr=%X\n"
                   "last=%016llX  ts=%016llX  ovh=%016llX  cnt=%X  sr=%X cnt=%X  sr=%X\n",
                    prec,precInfo.ts,precInfo.ovf,precInfo.cnt,precInfo.sr,precInfo.cntFirstUIF, precInfo.srFirstUIF,
                    last,lastInfo.ts,lastInfo.ovf,lastInfo.cnt,lastInfo.sr,lastInfo.cntFirstUIF, lastInfo.srFirstUIF );
        #endif//TIMER DEBUG
    }
}

static void inline testVhtEvent()
{
    puts("---------------Test timer VHT wait for event----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::OUTPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
    greenLed::high();
    for(;;)
    {
        blueLed::low();
        userButton::low();
        unsigned int delay = rand() % 3000 + 1;
        miosix::delayUs(delay);
        blueLed::high();
        userButton::high();
        miosix::delayUs(10);
    }
}

static void inline testVhtTriggerEvent()
{
    puts("---------------Test timer VHT trigger event----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::INPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
    greenLed::high();
    Timer& vht=VHT::instance();
    vht.setValue(0); //mezzo secondo 
    printf("Timestamp:  %016llX\n",vht.getValue());
    int i=1;
    for(;;)
    {        
        vht.setAbsoluteTriggerEvent(vhtFreq*i); //un secondo
        vht.wait();
       
        printf("Timestamp:  %016llX\n",vht.getValue());
        i++;
    }
}


static void inline testVhtTriggerEventOscilloscope()
{
    puts("---------------Test timer VHT trigger event----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::INPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
    greenLed::high();
    Timer& vht=VHT::instance();
    vht.setValue(0); //mezzo secondo 
    printf("Timestamp:  %016llX\n",vht.getValue());
    int i=1;
    for(;;)
    {        
        vht.setAbsoluteTriggerEvent(0.001*vhtFreq*i); // 1 ms
        vht.wait();
        i++;
    }
}

static void inline testOutputCompare()
{
    puts("---------------Test timer VHT output compare----------------");
    lowPowerSetup();
    blueLed::mode(miosix::Mode::INPUT);
    greenLed::mode(miosix::Mode::OUTPUT);
    userButton::mode(miosix::Mode::OUTPUT);
     typedef miosix::Gpio<GPIOB_BASE,1> trigger;
     trigger::mode(miosix::Mode::ALTERNATE);
    //enable TIM3
    {
        miosix::FastInterruptDisableLock dLock;
        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    }
    
    //TIM3 PARTIAL REMAP (CH1/PB4, CH2/PB5, CH3/PB0, CH4/PB1)
    AFIO->MAPR = AFIO_MAPR_TIM3_REMAP_1;
    
    TIM3->CNT=0;
    TIM3->PSC=2400-1;  //  24000000/24000000-1; //High frequency timer runs @24MHz
    TIM3->ARR=0xFFFF; //auto reload if counter register go in overflow
    TIM3->CR1=TIM_CR1_URS; 
    
    //setting output compare register1 for channel 1 and 2
    TIM3->CCMR2=TIM_CCMR2_OC4M_0 | TIM_CCMR2_OC4M_1; //toggle mode
    TIM3->CCR4=10000;  //set match register channel 1
    
    TIM3->CCER= TIM_CCER_CC4E; //enable channel
    TIM3->DIER=TIM_DIER_UIE;  //Enable interrupt event @ end of time to set flag
    TIM3->CR1|=TIM_CR1_CEN;
    NVIC_SetPriority(TIM3_IRQn,5); //Low priority
    NVIC_ClearPendingIRQ(TIM3_IRQn);
    NVIC_EnableIRQ(TIM3_IRQn);
    
    greenLed::high(); 
    for(;;)
    {
        miosix::delayMs(1000);
        printf("Timestamp:%X\n",TIM3->CNT);
    }
}



/*
 * 
 */
int main(int argc, char** argv) {
    //testRtc();
    //testRtcWaitExtEventOrTimeout();
    //testRtcTriggerEvent();
    //testVhtSleep();
    //testVhtGetPacketTimestamp();
    //testVhtWait();
    testVhtWaitThick();
    //testVhtWaitExtEventOrTimeout();
    //testVhtWaitExtEvent();
    //testVhtMonotonic();
    //testVhtEvent();
    //testVhtTriggerEvent();
    //testVhtTriggerEventOscilloscope();
    //testOutputCompare();
}
