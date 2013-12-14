/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WI<THOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/
#include "frame.h"
#include <iterator>
#include <cstring>
using namespace std;

//Frame::Frame() : pframe(NULL) {}

Frame::Frame(bool header, bool autoFCS, int sizeFCS) : pframe(NULL)
{
    if(sizeFCS*!autoFCS > MAX_SIZE_FCS) return;
    ctrlFrame.autoFcs = autoFCS;
    ctrlFrame.fcsLen = sizeFCS;
    ctrlFrame.header = header; 
}

Frame::Frame(int sizePayload, bool header, bool autoFCS, int sizeFCS) : pframe(NULL)
{
    int len = SIZE_LEN+sizePayload+header*SIZE_HEADER+!autoFCS*sizeFCS+autoFCS*SIZE_AUTO_FCS;
    if(sizeFCS*!autoFCS > MAX_SIZE_FCS || len > MAX_SIZE_FRAME) return;
    pframe = new unsigned char[len-autoFCS*SIZE_AUTO_FCS]();
    pframe[0] = len;
    ctrlFrame.autoFcs = autoFCS;
    ctrlFrame.fcsLen = autoFCS? SIZE_AUTO_FCS : sizeFCS;
    ctrlFrame.header = header; 
}

Frame::Frame(int sizePayload, const unsigned char* ppayload) : pframe(NULL) 
{
    if(sizePayload > MAX_SIZE_FRAME - SIZE_LEN - SIZE_AUTO_FCS) return;
    pframe = new unsigned char[SIZE_LEN + sizePayload]();
    pframe[0] = sizePayload + SIZE_LEN + SIZE_AUTO_FCS;
    ctrlFrame.autoFcs = 1;
    ctrlFrame.fcsLen = SIZE_AUTO_FCS;
    ctrlFrame.header = 0; 
    setPayload(ppayload);
}

Frame::Frame(const Frame& frame) : pframe(NULL)
{
    unsigned char size = frame.getFrameSize();
    pframe = new unsigned char[size]();
    ctrlFrame = frame.ctrlFrame;
    memcpy(this->pframe,frame.pframe,size);
}

bool Frame::operator==(const Frame& frame)const
{
    if(ctrlFrame != frame.ctrlFrame) return false;
    if(isNotInit() && frame.isNotInit()) return true;
    if(isNotInit() || frame.isNotInit()) return false;
    return memcmp(this->pframe,frame.pframe,this->getFrameSize())==0? true : false;
}

bool Frame::operator!=(const Frame& frame)const
{
    if (ctrlFrame != frame.ctrlFrame) return true;
    if(isNotInit() != frame.isNotInit()) return true;
    if(isNotInit() && frame.isNotInit()) return false;
    return memcmp(this->pframe,frame.pframe,this->getFrameSize())==0? false : true;
}

const Frame& Frame::operator=(const Frame& frame) 
{  
    ctrlFrame.autoFcs = frame.ctrlFrame.autoFcs;
    ctrlFrame.fcsLen = frame.ctrlFrame.fcsLen;
    ctrlFrame.header = frame.ctrlFrame.header; 
    if(frame.isNotInit()) return *this;
    unsigned char size = frame.getFrameSize();
    pframe = new unsigned char[size]();
    memcpy(frame.pframe,pframe,size);
    return *this;
}
/**
const unsigned char& Frame::operator[](int idx) const {
    return isNotInit() ? 0 : pframe[idx];
}
unsigned char& Frame::operator[](int& idx) {
    return isNotInit() ? 0x0 : pframe[idx];
}
*/


Frame::Iterator Frame::begin() 
{ 
    return isNotInit() ? NULL : Iterator(pframe+1); //don't allow write the LEN field
}

Frame::Iterator Frame::end() 
{
    return isNotInit() ?  NULL : Iterator(pframe + getFrameSize());
}

Frame::ConstIterator Frame::begin() const 
{
    return isNotInit() ?  NULL : ConstIterator(pframe);
}

Frame::ConstIterator Frame::end() const 
{
    return isNotInit() ?  NULL : ConstIterator(pframe + getFrameSize());
}

bool Frame::initFrame(int sizeLEN)
{
    if(!isNotInit()) return false;
    pframe = new unsigned char[sizeLEN-ctrlFrame.autoFcs*SIZE_AUTO_FCS]();
    pframe[0] = sizeLEN;
    return true;
}

int Frame::getFrameSize() const
{
    if(isNotInit()) return -1;
    return ctrlFrame.autoFcs==1? pframe[0]-2 : pframe[0]; 
}

int Frame::getLEN() const
{
    return isNotInit()? 0 : pframe[0];
}

bool Frame::setFCF(const unsigned char* fcf) 
{
    if(isNotInit() || fcf==NULL || !ctrlFrame.header) return false;
    pframe[FCF] = fcf[0];
    pframe[FCF+1] = fcf[1];
    return true;
}

bool Frame::getFCF(unsigned char* fcf) const
{
    if(isNotInit() || fcf==NULL || !ctrlFrame.header) return false;
    fcf[0] = pframe[FCF];
    fcf[1] = pframe[FCF+1];
    return true;
}

bool Frame::setSEQ(unsigned char seq)
{
    if(isNotInit() || !ctrlFrame.header) return false;
    pframe[SEQ] = seq;
    return true;
}

bool Frame::getSEQ(unsigned char& seq) const
{
    if(isNotInit() || !ctrlFrame.header) return false;
    seq = pframe[SEQ];
    return true;
}

bool Frame::setSourcePanId(const unsigned char* sourcePanId)
{
    if (isNotInit() || sourcePanId == NULL || !ctrlFrame.header) return false;

    pframe[SOURCE_PANID] = sourcePanId[0];
    pframe[SOURCE_PANID + 1] = sourcePanId[1];
    return true;
}

bool Frame::getSourcePanId(unsigned char* sourcePanId) const
{
    if(isNotInit() || sourcePanId==NULL || !ctrlFrame.header) return false;
        sourcePanId[0] = pframe[SOURCE_PANID];
        sourcePanId[1] = pframe[SOURCE_PANID+1];
        return true;
}

bool Frame::setDestPanId(const unsigned char* destPanId)
{
    if(isNotInit() || destPanId==NULL || !ctrlFrame.header) return false;
        pframe[DEST_PANID] = destPanId[0];
        pframe[DEST_PANID+1] = destPanId[1];
        return true;
}

bool Frame::getDestPanId(unsigned char* destPanId) const
{
    if(isNotInit() || destPanId==NULL || !ctrlFrame.header) return false;
        destPanId[0] = pframe[DEST_PANID];
        destPanId[1] = pframe[DEST_PANID+1];
        return true;
}

bool Frame::setSourceShortAddr(const unsigned char* sourceShrortAddr)
{
    if(isNotInit() || sourceShrortAddr==NULL || !ctrlFrame.header) return false;
        pframe[SOURCE_SHORT_ADDR] = sourceShrortAddr[0];
        pframe[SOURCE_SHORT_ADDR+1] = sourceShrortAddr[1];
        return true;
}

bool Frame::getSourceShortAddr(unsigned char* sourceShrortAddr) const
{
    if(isNotInit() || sourceShrortAddr==NULL || !ctrlFrame.header) return false;
    
        sourceShrortAddr[0] = pframe[SOURCE_SHORT_ADDR];
        sourceShrortAddr[1] = pframe[SOURCE_SHORT_ADDR + 1];
        return true;
}

bool Frame::setDestShortAddr(const unsigned char* destShrortAddr)
{
    if(isNotInit() || destShrortAddr==NULL || !ctrlFrame.header) return false;
        pframe[DEST_SHORT_ADDR] = destShrortAddr[0];
        pframe[DEST_SHORT_ADDR+1] = destShrortAddr[1];
        return true;
}

bool Frame::getDestShortAddr(unsigned char* destShrortAddr) const
{
    if(isNotInit() || destShrortAddr==NULL || !ctrlFrame.header) return false;
        destShrortAddr[0] = pframe[DEST_SHORT_ADDR];
        destShrortAddr[1] = pframe[DEST_SHORT_ADDR+1];
        return true;
}

bool Frame::setHeader(const unsigned char* header)
{
    if(isNotInit() || header==NULL || !ctrlFrame.header) return false;
        memcpy(pframe+HEADER,header,SIZE_HEADER);
        return true;
}

bool Frame::getHeader(unsigned char* header) const
{
    if(isNotInit() || header==NULL || !ctrlFrame.header) return false;
        memcpy(header,pframe+HEADER,SIZE_HEADER);
        return true;
}

bool Frame::haveHeader() const
{
    return ctrlFrame.header;
}

bool Frame::setPayload(const unsigned char* payload)
{
    if(isNotInit() || payload==NULL) return false;
    unsigned char sizePayload = getSizePayload();
    memcpy(pframe+SIZE_LEN+ctrlFrame.header*SIZE_HEADER,payload,sizePayload);
    return true;
}

bool Frame::getPayload(unsigned char* payload) const
{
    if(isNotInit() || payload==NULL) return false;
    unsigned char sizeHeader = getSizePayload();
    memcpy(payload,pframe+SIZE_LEN+ctrlFrame.header*SIZE_HEADER,sizeHeader);
    return true;
}

int Frame::getSizePayload() const
{
    if(isNotInit()) return 0;
    return getLEN()-SIZE_LEN-ctrlFrame.fcsLen-ctrlFrame.header*SIZE_HEADER;  
}


bool Frame::setFCS(const unsigned char* fcs)
{
    if(isNotInit() || fcs==NULL || ctrlFrame.autoFcs) return false;
    unsigned char size = (unsigned char)getFrameSize();
        memcpy(pframe+size-ctrlFrame.fcsLen,fcs,ctrlFrame.fcsLen);
        return true;
}

bool Frame::getFCS(unsigned char* fcs) const
{
    if(isNotInit() || fcs==NULL || ctrlFrame.autoFcs) return false;
        unsigned char size = (unsigned char)getFrameSize();
        memcpy(fcs,pframe+size-ctrlFrame.fcsLen,ctrlFrame.fcsLen);
        return true;
}



int Frame::getSizeFCS() const
{
    return ctrlFrame.fcsLen;
}

bool Frame::isAutoFCS() const
{
    return ctrlFrame.autoFcs;
}

bool Frame::isNotInit() const
{
    return pframe==NULL ? true : false;
}

Frame::~Frame() {
    delete [] pframe;
}

