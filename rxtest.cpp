
// Simple example that just sits there and prints every packet it receives

#include <cstdio>
#include <miosix.h>
#include "drivers/cc2520.h"
#include "drivers/timer.h"
#include "drivers/leds.h"
#include "flopsync_v3/protocol_constants.h"

int main()
{
    lowPowerSetup();
    led1::mode(miosix::Mode::OUTPUT);

    #ifdef USE_VHT
    Timer& timer=VHT::instance();
    #else //USE_VHT
    Timer& timer=Rtc::instance();
    #endif //USE_VHT

    Cc2520& transceiver=Cc2520::instance();
    transceiver.setFrequency(2450);
    transceiver.setMode(Cc2520::RX);
    transceiver.setAutoFCS(true);

    for(;;)
    {
        if(transceiver.isRxFrameDone()==1)
        {
            led1::high();
            unsigned char frame[127];
            unsigned char length=sizeof(frame);
            transceiver.readFrame(length,frame);
            printf("Received packet with RSSI=%ddBm\n",transceiver.getRssi());
            miosix::memDump(frame,length);
            puts("");
            led1::low();
        } else miosix::Thread::sleep(10);
    }
}
