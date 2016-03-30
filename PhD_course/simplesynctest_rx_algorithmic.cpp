

//
// code for PhD course
//

#include "simplesynctest.h"

int main()
{
    simpleInit();
    
    for(;;)
    {
        long long packetContent, whenReceived;
        simpleRecvPacket(packetContent,whenReceived);
        simpleSetTime(packetContent+simplePayloadTime);
        printf("Received %lld @ %lld, delta=%lld\n",
               packetContent,whenReceived,whenReceived-packetContent);
    }
}
