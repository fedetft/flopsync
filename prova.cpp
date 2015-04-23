
#include "BarraLed.h"
#include <iostream>

using namespace std;

int main()
{
    packet16 p;

    for(int i=0; i<32; i++)
    {
        p.encode(i);
        p.print();

        pair<int,bool> x = p.decode();

        cout << x.first << " " << boolalpha << x.second << endl;
    }
    return 0;
}
