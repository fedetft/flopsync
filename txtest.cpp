
// Simple example that just sends a packet roughly every second

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
	transceiver.setMode(Cc2520::TX);
	transceiver.setAutoFCS(true);

	for(;;)
	{
		miosix::ledOn();
		const unsigned char frame[]="Test message";
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
		miosix::ledOff();
		miosix::Thread::sleep(1000);
	}
}
