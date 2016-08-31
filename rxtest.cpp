
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

	bool first=true;
	int last=0;
	int total=0;
	int successful=0;
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
			if(length==120)
			{
				bool error=false;
				for(int j=2;j<length;j++) if(frame[j]!=j) error=true;
				int i=static_cast<int>(frame[0])<<8 | frame[1];
				if(error==false && i>last)
				{
					if(first)
					{
						first=false;
						last=i;
					} else {
						total+=i-last;
						last=i;
						successful++;
					}
					printf("correct packet with sequence %d received\n",i);
				} else printf("packet content is not correct\n");
			} else printf("length=%d is not correct\n",length);
			if(total>0)
				printf("received %d out of %d packets reception rate %5.1f%%\n",
					   successful,total,
					   static_cast<float>(successful)/static_cast<float>(total)*100);
            puts("");
            led1::low();
        } else miosix::Thread::sleep(10);
    }
}
