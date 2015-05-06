
#include "rtt_measure.h"

using namespace std;

RttMeasure::RttMeasure(char hopCount, Cc2520& transceiver, Timer& timer) : hopCount(hopCount), transceiver(transceiver), timer(timer)
{
    
}

std::pair<int, int> RttMeasure::rttClient(unsigned long long frameStart)
{       
        //
        // send RTT request packet
        //
        
        transceiver.setMode(Cc2520::TX);
        transceiver.setAutoFCS(false);

        unsigned char data[2];
        data[0] = 0x55;
        data[1] = hopCount;
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
        
//         iprintf("to=%d b0=%d b1=%d b2=%d\n",timeout,b0,b1,b2);
        
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
                iprintf("Bad packet received (readFrame returned %d)\n",retVal);
                miosix::memDump(pkt.getPacket(),len);
                            
            } else            
                return make_pair<int, int>(static_cast<int>(T2-T1),result.second);
        }
}
    
void RttMeasure::rttServer(unsigned long long frameStart, int cumulatedPropagationDelay)
{           
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
            
            char remoteHopCount = recv[1];
            
            if(retVal!=1 || len!=2 || recv[0]!=0x55 || (remoteHopCount - 1) != hopCount)
            {
                iprintf("Bad packet received (readFrame returned %d)\n",retVal);
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
                
                iprintf("RTT ok\n");
                //iprintf("timeout=%d b0=%d b1=%d b2=%d\n",timeout,b0,b1,b2);
            }
        }
}