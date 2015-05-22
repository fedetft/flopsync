#ifndef BARRALED_H
#define BARRALED_H

#include <utility>
#include <algorithm>
#include <cstring>

#ifdef _MIOSIX
#include "util/util.h"
#else // _MIOSIX

namespace miosix {
void memDump(const void *start, int len);
}
#endif // _MIOSIX

template<unsigned N>
class BarraLed
{
public:
    void encode(int num);
    std::pair<int,bool> decode();
    unsigned char* getPacket() { return packet; }
    void print();

    inline int getPacketSize() { return N; }
    
    static const int threshold = 6;

private:
    unsigned char packet[N];
};

template<unsigned N>
void BarraLed<N>::encode(int num)
{
    using namespace std;
    num = max<int>(0,min<int>(N*2,num));
    memset(packet, 0, N);

    for(int i = 0; i < num/2; i++)
        packet[i] = 0xff;

    if(num & 1) packet[num/2] = 0xf0;
}

template<unsigned N>
std::pair<int,bool> BarraLed<N>::decode()
{

    using namespace std;

    const int pktLenNibble = 2*N;

    int fromLeft=pktLenNibble-1;
    bool twoInaRow=false;
    for(int i=0;i<pktLenNibble;i++)
    {
        unsigned char mask = (i & 1) ? 0x0f : 0xf0;

        if((packet[i/2] & mask) != mask)
        {
            if(twoInaRow)
            {
                fromLeft=i-2;
                twoInaRow=false;
                break;
            } else twoInaRow=true;
        } else twoInaRow=false;
    }
    //Last nibble was different, compensate for that
    if(twoInaRow) fromLeft--;

    int fromRight=0;
    twoInaRow=false;

    for(int i=pktLenNibble-1;i>=0;i--)
    {
        unsigned char mask = (i & 1) ? 0x0f : 0xf0;
        if((packet[i/2] & mask) != 0x00)
        {
            if(twoInaRow)
            {
                fromRight=i+2;
                twoInaRow=false;
                break;
            } else twoInaRow=true;
        } else twoInaRow=false;
    }
    //Last nibble was different, compensate for that
    if(twoInaRow) fromRight++;

//     cerr<<fromLeft<<" "<<fromRight<<endl;
//     if(fromLeft>=fromRight) cout<<fromLeft<<" "<<fromRight<<" "<<packet<<endl;
//     assert(fromLeft<fromRight);
    if(fromRight-fromLeft>threshold) return make_pair(0,false);
    else return make_pair((fromLeft+fromRight+1)/2,true);
}

template<unsigned N>
void BarraLed<N>::print() { miosix::memDump(packet, N); }

typedef BarraLed<16> packet16;
typedef BarraLed<64> packet64;
typedef BarraLed<127> packet127;

#endif // BARRALED_H
