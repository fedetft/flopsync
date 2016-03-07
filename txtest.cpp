
// Simple example that just sends a packet roughly every second

#include <cstdio>
#include <miosix.h>
#include "drivers/cc2520.h"
#include "drivers/timer.h"
#include "flopsync_v3/protocol_constants.h"

int main()
{
    lowPowerSetup();

    #ifdef USE_VHT
    Timer& timer=VHT::instance();
    #else //USE_VHT
    Timer& timer=Rtc::instance();
    #endif //USE_VHT

    Cc2520& transceiver=Cc2520::instance();
    transceiver.setFrequency(2450);
    transceiver.setMode(Cc2520::TX);
    transceiver.setTxPower(Cc2520::P_5); puts("5dBm power");
    transceiver.setAutoFCS(true);

    for(int i=0;;i++)
    {
        miosix::ledOn();
        puts("Sending");
        unsigned char frame[120];
        frame[0]=(i>>8) & 0xff;
        frame[1]=     i & 0xff;
        for(int j=2;j<sizeof(frame);j++) frame[j]=j;
        transceiver.writeFrame(sizeof(frame),frame);
        //Need to some slack time from now to the time when the trigger is sent.
        //(1ms is a *very* conservative value)
        unsigned long long t=timer.getValue()+static_cast<unsigned long long>(0.001f*hz);
        timer.absoluteWaitTrigger(t);
        //Need to add a timeout for the event (10ms is a *very* conservative value)
        timer.absoluteWaitTimeoutOrEvent(t+static_cast<unsigned long long>(0.01f*hz));
        transceiver.isSFDRaised();
        timer.absoluteWaitTimeoutOrEvent(t+static_cast<unsigned long long>(0.01f*hz));
        transceiver.isTxFrameDone();
        miosix::memDump(frame,sizeof(frame));
        puts("Sent");
        miosix::ledOff();
        miosix::Thread::sleep(1000);
    }
}
