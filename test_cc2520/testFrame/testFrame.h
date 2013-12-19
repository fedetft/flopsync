/*
 * File:   TestFrame.h
 * Author: gigi
 *
 * Created on Dec 11, 2013, 10:33:56 AM
 */

#ifndef TESTFRAME_H
#define	TESTFRAME_H

#define PRINT
#include <cstdlib>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include "../../drivers/frame.h"
#include <cppunit/extensions/HelperMacros.h>

class TestFrame : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestFrame);

    CPPUNIT_TEST(testConstructors);
    CPPUNIT_TEST(testInitFrame);
    CPPUNIT_TEST(testGetFrameSize);
    CPPUNIT_TEST(testGetLen);
    CPPUNIT_TEST(testSetFCF);
    CPPUNIT_TEST(testGetFCF);
    CPPUNIT_TEST(testSetSEQ);
    CPPUNIT_TEST(testGetSEQ);
    CPPUNIT_TEST(testSetSourcePanId);
    CPPUNIT_TEST(testGetSourcePanId);
    CPPUNIT_TEST(testSetSourceShortAdrr);
    CPPUNIT_TEST(testGetSourceShortAdrr);
    CPPUNIT_TEST(testSetDestPanId);
    CPPUNIT_TEST(testGetDestPanId);
    CPPUNIT_TEST(testSetDestShortAdrr);
    CPPUNIT_TEST(testGetDestShortAdrr);
    CPPUNIT_TEST(testSetHeader);
    CPPUNIT_TEST(testGetHeader);
    CPPUNIT_TEST(testHaveHeader);
    CPPUNIT_TEST(testSetPayload);
    CPPUNIT_TEST(testGetPayload);
    CPPUNIT_TEST(testGetSizePayload);
    CPPUNIT_TEST(testSetFCS);
    CPPUNIT_TEST(testGetFCS);
    CPPUNIT_TEST(isAutoFCS);

    CPPUNIT_TEST_SUITE_END();

public:
    TestFrame();
    virtual ~TestFrame();
    void setUp();
    void tearDown();

private:
    void testConstructors();
    void testInitFrame();
    void testGetFrameSize();
    void testGetLen();
    void testSetFCF();
    void testGetFCF();
    void testSetSEQ();
    void testGetSEQ();
    void testSetSourcePanId();
    void testGetSourcePanId();
    void testSetSourceShortAdrr();
    void testGetSourceShortAdrr();
    void testSetDestPanId();
    void testGetDestPanId();
    void testSetDestShortAdrr();
    void testGetDestShortAdrr();
    void testSetHeader();
    void testGetHeader();
    void testHaveHeader();
    void testSetPayload();
    void testGetPayload();
    void testGetSizePayload();
    void testSetFCS();
    void testGetFCS();
    void isAutoFCS();
    
    
    unsigned char fcf[2] = {0xC,0xD};
    unsigned char seq = 0x10;
    unsigned char sourcePanId[2] = {0x01,0x01};
    unsigned char destPanId[2] = {0x02,0x02};
    unsigned char sourceShortAddr[2] = {0xA,0x0A};
    unsigned char destShortAddr[2] = {0x0B,0x0B};
    unsigned char header[11] = {0xC,0xD,0x10,0x01,0x01,0xA,0x0A,0x02,0x02,0x0B,0x0B};
    unsigned char FCS = 0x7A;
    unsigned char *fcs = &FCS;
    unsigned char payload[5] = {'F','R','A','M','E'};
    
    int frame1LEN;
    int frame2LEN;
    int frame3LEN;
    int frame4LEN;
    
    int frame1Size;
    int frame2Size;
    int frame3Size;
    int frame4Size;
    
    Frame *frame1; //get Frame
    Frame *frame2; //get Frame
    Frame *frame3; //setting Frame
    Frame *frame4; //not init Frame
    
    void printFrame(const Frame& frame) const 
    {
        #ifdef PRINT
            Frame::ConstIterator b = frame.begin();
            Frame::ConstIterator e = frame.end();
            std::cout << "Size iterator: " << e - b << std::endl;
            std::cout << "Frame read: ";
            for (Frame::ConstIterator i = b; i < e; i++)
                std::printf("/%X",*i);
            std::cout << std::endl;
        #endif //PRINT
    }
};

#endif	/* TESTFRAME_H */

