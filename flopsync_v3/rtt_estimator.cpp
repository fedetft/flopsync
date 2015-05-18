
#include "rtt_estimator.h"

using namespace std;

typedef miosix::Gpio<GPIOC_BASE,8> blueLed;

/**
 * Temporarily disable VHT to RTC resync. Also, turn on blue led.
 */
class VHTScopedUnsync
{
public:
    VHTScopedUnsync(Timer& timer);
    ~VHTScopedUnsync();
private:
    VHT *vht;
    bool syncWasEnabled;
};

VHTScopedUnsync::VHTScopedUnsync(Timer& timer) : vht(dynamic_cast<VHT*>(&timer))
{
    blueLed::high();
    if(!vht) return;
    syncWasEnabled=vht->isAutoSync();
    if(syncWasEnabled) vht->disableAutoSyncWithRtc();
}

VHTScopedUnsync::~VHTScopedUnsync()
{
    if(vht && syncWasEnabled) vht->enableAutoSyncWhitRtc();
    blueLed::low();
}

//
// class RttEstimator
//

RttEstimator::RttEstimator(char hopCount, Cc2520& transceiver, Timer& timer) : hopCount(hopCount), transceiver(transceiver), timer(timer)
{
    blueLed::mode(miosix::Mode::OUTPUT);
}

std::pair<int, int> RttEstimator::rttClient(unsigned long long frameStart)
{
    long long wakeupTime=frameStart-(jitterAbsorption+txTurnaroundTime);
    timer.absoluteSleep(wakeupTime);
    
    VHTScopedUnsync unsync(timer);
    //
    // send RTT request packet
    //
    
    transceiver.setMode(Cc2520::TX);
    transceiver.setAutoFCS(false);
    
    unsigned char data[2];
    data[0] = 0x55;
    data[1] = hopCount-1;
    unsigned char len = sizeof(data);
    transceiver.writeFrame(len,data);       //send RTT packet
    timer.absoluteWaitTrigger(frameStart-txTurnaroundTime);
    bool b0=timer.absoluteWaitTimeoutOrEvent(frameStart+preambleFrameTime+delaySendPacketTime);
    unsigned long long T1=timer.getExtEventTimestamp();
    transceiver.isSFDRaised();
    bool b1=timer.absoluteWaitTimeoutOrEvent(frameStart+rttPacketTime+delaySendPacketTime);
    transceiver.isTxFrameDone();
        
    //
    // await RTT response packet
    //
    
    packet16 pkt;
    transceiver.setMode(Cc2520::RX);
    transceiver.setAutoFCS(false);
    bool timeout=timer.absoluteWaitTimeoutOrEvent(
        frameStart+preambleFrameTime+rttRetransmitTime+preambleFrameTime+delaySendPacketTime);//had +txTurnaroundTime
    unsigned long long T2=timer.getExtEventTimestamp();
    transceiver.isSFDRaised();
    bool b2=timer.absoluteWaitTimeoutOrEvent(
        frameStart+preambleFrameTime+rttRetransmitTime+preambleFrameTime+rttResponseTailPacketTime+delaySendPacketTime);//had +txTurnaroundTime
    transceiver.isRxFrameDone();      
    
    //
    // process received data
    //
    
    //iprintf("to=%d b0=%d b1=%d b2=%d\n",timeout,b0,b1,b2);
    int lastDelay=-1, cumulatedDelay=-1;
    if(timeout)
    {
        iprintf("Timeout while waiting RTT response\n");
    } else {
        iprintf("T1 absolute value: %llu\n",T1);
        iprintf("T2 absolute value: %llu\n",T2);
        len=pkt.getPacketSize();
        int retVal=transceiver.readFrame(len,pkt.getPacket());
        std::pair<int,bool> result=pkt.decode();
        
        if(retVal!=1 || len!=pkt.getPacketSize() || result.second==false)
        {
            iprintf("Bad packet received  while waiting RTT response (readFrame returned %d)\n",retVal);
            miosix::memDump(pkt.getPacket(),len);
        } else {
            int delta=static_cast<int>(T2-T1);
            const int t1tot2nominal=rttRetransmitTime+preambleFrameTime+2*trasmissionTime;
            lastDelay=max(0,(delta-t1tot2nominal)/2-rttOffsetTicks);
            cumulatedDelay=result.first;
        }
    }
    transceiver.setMode(Cc2520::DEEP_SLEEP);
    return make_pair<int, int>(lastDelay, cumulatedDelay);
}
    
void RttEstimator::rttServer(unsigned long long frameStart, int cumulatedPropagationDelay)
{
    unsigned long long wakeupTime=frameStart-(jitterAbsorption+rxTurnaroundTime+w);
    timer.absoluteSleep(wakeupTime);
    
    VHTScopedUnsync unsync(timer);
    //
    // await RTT request packet
    //

    transceiver.setMode(Cc2520::RX);
    transceiver.setAutoFCS(false);
    bool timeout=timer.absoluteWaitTimeoutOrEvent(frameStart+rttPacketTime);
    unsigned long long measuredTime=timer.getExtEventTimestamp();
    transceiver.isSFDRaised();
    bool b0=timer.absoluteWaitTimeoutOrEvent(frameStart+rttPacketTime+rttTailPacketTime+delaySendPacketTime);
    transceiver.isRxFrameDone();
    
    if(timeout)
    {
        iprintf("Timeout while waiting RTT request\n");
                
    } else {
    
        unsigned char recv[2];
        unsigned char len = sizeof(recv);
        int retVal = transceiver.readFrame(len,recv);
        
        if(retVal!=1 || len!=2 || recv[0]!=0x55 || recv[1] != hopCount)
        {
            iprintf("Bad packet received while waiting RTT request (readFrame returned %d)\n",retVal);
            miosix::memDump(recv,len);
                        
        } else {
            
            //
            // send RTT response packet
            //
            
            transceiver.setMode(Cc2520::TX);
            transceiver.setAutoFCS(false);
        
            packet16 pkt;
            pkt.encode(cumulatedPropagationDelay);
            len=pkt.getPacketSize();
        
            transceiver.writeFrame(len,pkt.getPacket());
            timer.absoluteWaitTrigger(measuredTime+rttRetransmitTime-txTurnaroundTime);
            bool b1=timer.absoluteWaitTimeoutOrEvent(measuredTime+rttRetransmitTime+preambleFrameTime+delaySendPacketTime);
            transceiver.isSFDRaised();
            bool b2=timer.absoluteWaitTimeoutOrEvent(measuredTime+rttRetransmitTime+preambleFrameTime+rttResponseTailPacketTime+delaySendPacketTime);
            transceiver.isTxFrameDone();
            
            //iprintf("RTT ok\n");
            //iprintf("timeout=%d b0=%d b1=%d b2=%d\n",timeout,b0,b1,b2);
        }
    }
    
    transceiver.setMode(Cc2520::DEEP_SLEEP);
}