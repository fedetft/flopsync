/*
 * File:   TestFrame.cpp
 * Author: gigi
 *
 * Created on Dec 11, 2013, 10:33:57 AM
 */

#include "testFrame.h"

using namespace std;



CPPUNIT_TEST_SUITE_REGISTRATION(TestFrame);

TestFrame::TestFrame()
{
}

TestFrame::~TestFrame()
{
}

void TestFrame::setUp()
{
    #ifdef PRINT
        cout << endl<< " ----Set UP------ " << endl;
    #endif
    
    int sp = sizeof(payload)/(sizeof(payload[0]));
    
    frame1 = new Frame(sp,true,false,1);
    frame2 = new Frame(sp,payload);
    frame3 = new Frame(sp,true,false,1);
    frame4 = new Frame(false,true,2);
    
    
    
    frame1->setHeader(header);
    frame1->setPayload(payload);
    frame1->setFCS(fcs);
    
    
    
    
    frame1LEN = SIZE_HEADER + sp + 1;
    frame2LEN = sp + SIZE_AUTO_FCS;
    frame3LEN = SIZE_HEADER + sp + 1;
    frame4LEN = sp + SIZE_AUTO_FCS;
    
    frame1Size = SIZE_LEN + frame1LEN;
    frame2Size = SIZE_LEN + frame2LEN - SIZE_AUTO_FCS;
    frame3Size = SIZE_LEN + frame3LEN;
    frame4Size = SIZE_LEN + frame4LEN - SIZE_AUTO_FCS;
       

}

void TestFrame::tearDown()
{
    #ifdef PRINT
        cout << " ----Tear Down------ " << endl;
    #endif
    delete frame1;
    delete frame2;
    delete frame3;
    delete frame4;
}

void TestFrame::testConstructors()
{
    int payload;
    int sizeFcs;
    int header;
    int autoFcs;
    srand(time(0));
    for(int i=0; i<1000; i++)
    {
        payload = rand() % 300 + 1;
        sizeFcs = rand() % 10 + 1;
        header =  rand() % 2;
        autoFcs = rand() % 2;
        
        #ifdef PRINT
            cout << "  *****  " << endl;
            cout << "Payload: " << payload << endl;
            cout << "Header: " << header << endl;
            cout << "AutoFCS: " << autoFcs << endl;
            cout << "sizeFCS: " << sizeFcs << endl;
        #endif
        
        Frame *f = new Frame(false, true, 2);
        CPPUNIT_ASSERT(f->isNotInit());
        CPPUNIT_ASSERT(*f==*frame4);
        Frame *errF = new Frame(false, true, 15);
        CPPUNIT_ASSERT(errF->isNotInit());
        CPPUNIT_ASSERT(*errF!=*frame4);
        delete f;
        delete errF;
            
        Frame *f1 = new Frame(payload, header, autoFcs, sizeFcs);
        
        if ((payload + SIZE_HEADER * header +
                SIZE_AUTO_FCS * autoFcs + !autoFcs * sizeFcs) > MAX_SIZE_FRAME)
            CPPUNIT_ASSERT(f1->isNotInit());
        else
            if(!autoFcs*sizeFcs > 7)
                CPPUNIT_ASSERT(f1->isNotInit());
            else
                CPPUNIT_ASSERT(!f1->isNotInit());
      
        delete f1;
        
        unsigned char ppayload[payload];
        Frame *f2 = new Frame(payload,ppayload);
        
        if (payload + SIZE_AUTO_FCS > MAX_SIZE_FRAME)
            CPPUNIT_ASSERT(f2->isNotInit());
        else
            CPPUNIT_ASSERT(!f2->isNotInit());
      
        delete f2;
        
        Frame *f3 = new Frame(*frame1);
        Frame *f4;
        f4=f3;
        CPPUNIT_ASSERT(!f3->isNotInit());
        CPPUNIT_ASSERT(*f3==*frame1);
        CPPUNIT_ASSERT(!(*f3!=*frame1));
        CPPUNIT_ASSERT(*f4==*frame1);
        CPPUNIT_ASSERT(*f3==*frame1);
        CPPUNIT_ASSERT(*f3!=*frame2);
        
        delete f3;
    }
}

void TestFrame::testInitFrame()
{  
    frame4->initFrame(frame4LEN);
    Frame::Iterator begin = frame4->begin();
    Frame::Iterator end = frame4->end();
    CPPUNIT_ASSERT(begin!=NULL);
    CPPUNIT_ASSERT(end!=NULL);
    #ifdef PRINT
        cout << "Size iterator: " << end-begin <<endl;
        cout << "Frame writed: ";
    #endif
    unsigned char* pay = payload;
    for(Frame::Iterator i=begin; i<end; i++) {
        cout << *pay;
        *i=*pay++;
    }
    #ifdef PRINT
        cout << endl;
    #endif
    
    printFrame(*frame4);
        
    unsigned char p[frame4->getSizePayload()];
    CPPUNIT_ASSERT(frame4->getPayload(p));
    CPPUNIT_ASSERT(memcmp(p,payload,frame4->getSizePayload())==0); 
    
    
    Frame *f = new Frame(false);
    CPPUNIT_ASSERT(f->initFrame(frame2LEN));
    CPPUNIT_ASSERT(f->setPayload(payload));
    printf("frame size %d \n",f->getFrameSize());
    printFrame(*f);
    CPPUNIT_ASSERT(*f==*frame2);
    delete f;
    
}


void TestFrame::testGetLen()
{
    CPPUNIT_ASSERT(frame1->getLEN()==frame1LEN);
    CPPUNIT_ASSERT(frame2->getLEN()==frame2LEN);
    CPPUNIT_ASSERT(frame3->getLEN()==frame3LEN);
}

void TestFrame::testGetFrameSize()
{
    CPPUNIT_ASSERT(frame1->getFrameSize()==frame1Size);
    CPPUNIT_ASSERT(frame2->getFrameSize()==frame2Size);
    CPPUNIT_ASSERT(frame3->getFrameSize()==frame3Size);
}

void TestFrame::testSetFCF()
{
    CPPUNIT_ASSERT(frame3->setFCF(fcf));
    unsigned char p[2];
    CPPUNIT_ASSERT(frame3->getFCF(p));
    CPPUNIT_ASSERT(memcmp(p,fcf,2)==0);
    printFrame(*frame3);
    
}

void TestFrame::testGetFCF()
{
    unsigned char p[2];
    CPPUNIT_ASSERT(frame1->getFCF(p));
    CPPUNIT_ASSERT(memcmp(p,fcf,2)==0);
    
}

void TestFrame::testSetSEQ()
{
    unsigned char s;
    CPPUNIT_ASSERT(frame3->setSEQ(seq));
    CPPUNIT_ASSERT(frame3->getSEQ(s));
    CPPUNIT_ASSERT(seq==s);
    printFrame(*frame3);
}
void TestFrame::testGetSEQ()
{
    unsigned char s;
    CPPUNIT_ASSERT(frame1->getSEQ(s));
    CPPUNIT_ASSERT(seq==s);
}

void TestFrame::testSetSourcePanId()
{
    unsigned char p[2];
    CPPUNIT_ASSERT(frame3->setSourcePanId(sourcePanId));
    CPPUNIT_ASSERT(frame3->getSourcePanId(p));
    CPPUNIT_ASSERT(memcmp(p,sourcePanId,2)==0);
    printFrame(*frame3);
}

void TestFrame::testGetSourcePanId()
{
    unsigned char p[2];
    CPPUNIT_ASSERT(frame1->getSourcePanId(p));
    CPPUNIT_ASSERT(memcmp(p,sourcePanId,2)==0);
}

void TestFrame::testSetSourceShortAdrr()
{
    unsigned char p[2];
    CPPUNIT_ASSERT(frame3->setSourceShortAddr(sourceShortAddr));
    CPPUNIT_ASSERT(frame3->getSourceShortAddr(p));
    CPPUNIT_ASSERT(memcmp(p,sourceShortAddr,2)==0);
    printFrame(*frame3);
}

void TestFrame::testGetSourceShortAdrr()
{
    unsigned char p[2];
    CPPUNIT_ASSERT(frame1->getSourceShortAddr(p));
    CPPUNIT_ASSERT(memcmp(p,sourceShortAddr,2)==0);
}
void TestFrame::testSetDestPanId()
{
    unsigned char p[2];
    CPPUNIT_ASSERT(frame3->setDestPanId(destPanId));
    CPPUNIT_ASSERT(frame3->getDestPanId(p));
    CPPUNIT_ASSERT(memcmp(p,destPanId,2)==0);
    printFrame(*frame3);
}

void TestFrame::testGetDestPanId()
{
    unsigned char p[2];
    CPPUNIT_ASSERT(frame1->getDestPanId(p));
    CPPUNIT_ASSERT(memcmp(p,destPanId,2)==0);
}

void TestFrame::testSetDestShortAdrr()
{
    unsigned char p[2];
    CPPUNIT_ASSERT(frame3->setDestShortAddr(destShortAddr));
    CPPUNIT_ASSERT(frame3->getDestShortAddr(p));
    CPPUNIT_ASSERT(memcmp(p,destShortAddr,2)==0);
    printFrame(*frame3);
}

void TestFrame::testGetDestShortAdrr()
{
    unsigned char p[2];
    CPPUNIT_ASSERT(frame1->getDestShortAddr(p));
    CPPUNIT_ASSERT(memcmp(p,destShortAddr,2)==0);
}

void TestFrame::testSetHeader()
{
    unsigned char p[SIZE_HEADER];
    CPPUNIT_ASSERT(frame3->setHeader(header));
    CPPUNIT_ASSERT(frame3->getHeader(p));
    CPPUNIT_ASSERT(memcmp(p,header,SIZE_HEADER)==0);
    printFrame(*frame3);
}

void TestFrame::testGetHeader()
{
    unsigned char p[SIZE_HEADER];
    CPPUNIT_ASSERT(frame1->getHeader(p));
    CPPUNIT_ASSERT(memcmp(p,header,SIZE_HEADER)==0);
}
void TestFrame::testHaveHeader()
{
    CPPUNIT_ASSERT(frame1->haveHeader());
    CPPUNIT_ASSERT(!frame2->haveHeader());
    CPPUNIT_ASSERT(frame1->haveHeader());
}

void TestFrame::testSetPayload()
{
    unsigned char p[frame3->getSizePayload()];
    CPPUNIT_ASSERT(frame3->setPayload(payload));
    CPPUNIT_ASSERT(frame3->getPayload(p));
    CPPUNIT_ASSERT(memcmp(p,payload,frame3->getSizePayload())==0);
    printFrame(*frame3);
}

void TestFrame::testGetPayload()
{
    unsigned char p[frame1->getSizePayload()];
    CPPUNIT_ASSERT(frame1->getPayload(p));
    CPPUNIT_ASSERT(memcmp(p,payload,frame1->getSizePayload())==0);
}

void TestFrame::testGetSizePayload()
{
    CPPUNIT_ASSERT(frame1->getSizePayload()==sizeof(payload)/sizeof(payload[0]));
}
void TestFrame::testSetFCS()
{
    unsigned char p[1];
    CPPUNIT_ASSERT(frame3->setFCS(fcs));
    CPPUNIT_ASSERT(frame3->getFCS(p));
    CPPUNIT_ASSERT(memcmp(p,fcs,1)==0);
    printFrame(*frame3);
}
void TestFrame::testGetFCS()
{
    unsigned char p[1];
    CPPUNIT_ASSERT(frame1->getFCS(p));
    CPPUNIT_ASSERT(memcmp(p,fcs,1)==0);
}
void TestFrame::isAutoFCS()
{
    CPPUNIT_ASSERT(!frame1->isAutoFCS());
    CPPUNIT_ASSERT(frame2->isAutoFCS());
    CPPUNIT_ASSERT(!frame3->isAutoFCS());
}

