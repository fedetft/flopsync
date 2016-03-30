
//
// code for PhD course
//

#include "simplesynctest.h"

int main()
{
    simpleInit();
    
    long long time=simpleGetTime();
    
    for(;;)
    {
        time=time+nominalPeriod;
        simpleSendPacket(time,time);
        printf("Sent current time @ %lld\n",time);
    }
}
