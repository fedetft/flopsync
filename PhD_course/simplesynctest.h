
//
// code for PhD course
//

#ifndef SIMPLESYNCTEST_H
#define SIMPLESYNCTEST_H

#include "drivers/cc2520.h"
#include "drivers/timer.h"
#include "drivers/frame.h"
#include "drivers/leds.h"
#include "flopsync_v3/protocol_constants.h"
#include "flopsync_v3/flooder_root_node.h"
#include "flopsync_v3/synchronizer.h"
#include "flopsync_v3/rtt_estimator.h"
#include "drivers/BarraLed.h"
#include "board_setup.h"

inline void simpleInit()
{
    lowPowerSetup();
    miosix::redLed::mode(miosix::Mode::OUTPUT_LOW);
    
}

inline long long simpleGetTime()
{
    #ifndef USE_VHT
    return Rtc::instance().getValue();
    #else //USE_VHT
    return Rtc::instance().getValue();
    #endif //USE_VHT
}

inline void simpleSetTime(long long time)
{
    #ifndef USE_VHT
    Rtc::instance().setValue(time);
    #else //USE_VHT
    Rtc::instance().setValue(time);
    #endif //USE_VHT
}

inline void simpleWait(long long time)
{
    #ifndef USE_VHT
    Rtc::instance().absoluteWait(time);
    #else //USE_VHT
    Rtc::instance().absoluteWait(time);
    #endif //USE_VHT
}

struct SimplePacket
{
    long long payload;
    unsigned int magic;
};

const unsigned long long simplePacketTime=static_cast<unsigned long long>(((sizeof(SimplePacket)+8)*8*hz)/channelbps+0.5f); //+8 is 4=preamble 1=sfd 1=length 2=crc

const unsigned long long simplePayloadTime=static_cast<unsigned long long>(((sizeof(SimplePacket)+3)*8*hz)/channelbps+0.5f)+12; //+3 is 1=length 2=crc +12=empirically determined coefficient to compensate for other delays

const unsigned long long longEnoughDelay=static_cast<unsigned long long>(0.1f*hz+0.5f);

const unsigned int simplePacketMagic=0x58edfc90;

inline void simpleSendPacket(long long packetContent, long long whenToSend)
{
    Timer& timer=Rtc::instance();
    timer.absoluteSleep(whenToSend-longEnoughDelay);
    
    Cc2520& transceiver=Cc2520::instance();
    transceiver.setTxPower(Cc2520::P_5);
    transceiver.setFrequency(2450);
    transceiver.setAutoFCS(true);
    transceiver.setMode(Cc2520::TX);
    
    SimplePacket packet;
    packet.magic=simplePacketMagic;
    packet.payload=packetContent;
    
    unsigned char len=sizeof(SimplePacket);
    unsigned char *data=reinterpret_cast<unsigned char*>(&packet);
    transceiver.writeFrame(len,data);
    
    long long triggerTime=whenToSend-txTurnaroundTime;
    timer.absoluteWaitTrigger(triggerTime);
    miosix::redLed::high();
    timer.absoluteWaitTimeoutOrEvent(triggerTime+txTurnaroundTime+preambleFrameTime+delaySendPacketTime);
    transceiver.isSFDRaised();
    timer.absoluteWaitTimeoutOrEvent(triggerTime+txTurnaroundTime+simplePacketTime+delaySendPacketTime);
    transceiver.isTxFrameDone();
    miosix::redLed::low();
    transceiver.setMode(Cc2520::DEEP_SLEEP);
}

bool fixme(long long& packetContent, long long& whenReceived)
{
    Timer& timer=Rtc::instance();    
    Cc2520& transceiver=Cc2520::instance();
    transceiver.setTxPower(Cc2520::P_5);
    transceiver.setFrequency(2450);
    transceiver.setAutoFCS(true);
    transceiver.setMode(Cc2520::RX);
    
    unsigned long long measuredTime;
    SimplePacket packet;
//     for(;;)
//     {
        timer.absoluteWaitTimeoutOrEvent(0);
        measuredTime=timer.getExtEventTimestamp()-preambleFrameTime;
        miosix::redLed::high();
        transceiver.isSFDRaised();
        timer.absoluteWaitTimeoutOrEvent(measuredTime+simplePacketTime+delaySendPacketTime);
        transceiver.isRxFrameDone();
        miosix::redLed::low();
        
        unsigned char len=sizeof(SimplePacket);
        unsigned char *data=reinterpret_cast<unsigned char*>(&packet);
        
        bool success=true;
        if(transceiver.readFrame(len,data)!=1) success=false; //continue;
        if(len!=sizeof(SimplePacket)) success=false; //continue;
        if(packet.magic!=simplePacketMagic) success=false; //continue;
//         break;
//     }
    packetContent=packet.payload;
    whenReceived=measuredTime;
    transceiver.setMode(Cc2520::DEEP_SLEEP);
    return success;
}

inline void simpleRecvPacket(long long& packetContent, long long& whenReceived)
{
    for(;;)
    {
        if(fixme(packetContent,whenReceived)) break;
        miosix::Thread::sleep(10);
    }
}

#endif //SIMPLESYNCTEST_H
