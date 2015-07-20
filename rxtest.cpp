
// Simple example that just sits there and prints every packet it receives

#include <cstdio>
#include <miosix.h>
#include "drivers/cc2520.h"
#include "drivers/timer.h"

int main()
{
	lowPowerSetup();

	Timer& timer=VHT::instance();

	Cc2520& transceiver=Cc2520::instance();
	transceiver.setFrequency(2450);
	transceiver.setMode(Cc2520::RX);
	transceiver.setAutoFCS(false);

	for(;;)
	{
		if(transceiver.isRxFrameDone()==1)
		{
			miosix::ledOn();
			unsigned char frame[127];
			unsigned char length=sizeof(frame);
			transceiver.readFrame(length,frame);
			miosix::memDump(frame,length);
			miosix::ledOff();
		} else miosix::Thread::sleep(10);
	}
}
