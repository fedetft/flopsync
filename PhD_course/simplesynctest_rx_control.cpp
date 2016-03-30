

//
// code for PhD course
//

#include "simplesynctest.h"

int main()
{
    simpleInit();
    
    long long packetContent, whenReceived;
    simpleRecvPacket(packetContent,whenReceived);
    simpleSetTime(packetContent+simplePayloadTime);
    printf("Received %lld @ %lld, delta=%lld\n",
           packetContent,whenReceived,whenReceived-packetContent);
    
    float uo=0.f;
    float eo=0.f;
    
    for(;;)
    {
        simpleRecvPacket(packetContent,whenReceived);
        
        float e=whenReceived-packetContent;
        float u=uo-(8.f/5.f)*e+eo;
        eo=e;
        uo=u;
        
        simpleSetTime(simpleGetTime()+u); //FIXME: the implementation of simpleSetTime introduces jitter/hysteresis
        printf("Received %lld @ %lld, e=%1.0f u=%3.1f\n",
               packetContent,whenReceived,e,u);
    }
}
