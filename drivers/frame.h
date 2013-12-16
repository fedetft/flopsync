/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
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

#ifndef FRAME_H
#define	FRAME_H

//    _________________________________________________________________________
//   |      |     |     | SOURCE | SOURCE | DEST | DEST  |               |     |
//   | LEN  | FCF | SEQ | PAN    | SHORT  | PAN  | SHORT | ...PAYLOAD... | FCS |
//   |      |     |     | ID     | ADDR   | ID   | ADDR  |               |     |
//   |======|=====|=====|========|========|======|=======|===============|=====|
//   ^      ^     ^     ^        ^        ^      ^      ^               ^     ^  
//   0      1     3     4        6        8      10    12                     MAX_SIZE_FRAME

#define SIZE_LEN 1
#define SIZE_HEADER 11
#define MAX_SIZE_FRAME 127
#define SIZE_AUTO_FCS 2
#define MAX_SIZE_FCS 7
#define FCF 1
#define SEQ 3
#define SOURCE_PANID 4
#define SOURCE_SHORT_ADDR 6
#define DEST_PANID 8
#define DEST_SHORT_ADDR 10
#define HEADER 1


#include <iterator>


class Frame 
{
public:
    
    // ---------------------------------
    // Forward declaration
    // ---------------------------------
    class ConstIterator;

    // ---------------------------------
    // iterator class
    // ---------------------------------
    class Iterator: public std::iterator<std::random_access_iterator_tag, unsigned char>
    {
    public:
        Iterator(): p_(NULL) {}
        Iterator(unsigned char* p): p_(p) {}
        Iterator(const Iterator& other): p_(other.p_) {}
        Iterator& operator=(const Iterator& other) {p_ = other.p_; return *this;}

        Iterator& operator++()    {p_++; return *this;} // prefix++
        Iterator  operator++(int) {Iterator tmp(*this); ++(*this); return tmp;} // postfix++
        Iterator& operator--()    {p_--; return *this;} // prefix--
        Iterator  operator--(int) {Iterator tmp(*this); --(*this); return tmp;} // postfix--

        void     operator+=(const int& n)  {p_ += n;}
        Iterator operator+ (const int& n)  {Iterator tmp(*this); tmp += n; return tmp;}
        

        void        operator-=(const int& n)  {p_ -= n;}
        //void        operator-=(const Iterator& other) {p_ -= other.p_;}
        Iterator    operator- (const int& n)  {Iterator tmp(*this); tmp -= n; return tmp;}
        int         operator- (const Iterator& other) {return p_ - other.p_;}

        bool operator< (const Iterator& other) {return (p_-other.p_)< 0;}
        bool operator<=(const Iterator& other) {return (p_-other.p_)<=0;}
        bool operator> (const Iterator& other) {return (p_-other.p_)> 0;}
        bool operator>=(const Iterator& other) {return (p_-other.p_)>=0;}
        bool operator==(const Iterator& other) {return  p_ == other.p_; }
        bool operator!=(const Iterator& other) {return  p_ != other.p_; }

        unsigned char& operator[](const int& n) {return *(p_+n);}
        unsigned char& operator*() {return *p_;}
        unsigned char* operator->(){return  p_;}
    private:
        unsigned char* p_;

        friend class ConstIterator;
    };

    // ---------------------------------
    // ConstIterator class
    // ---------------------------------
    class ConstIterator: public std::iterator<std::random_access_iterator_tag, unsigned char>
    {
    public:
        ConstIterator(): p_(NULL) {}
        ConstIterator(const unsigned char* p): p_(p) {}
        ConstIterator(const Iterator& other): p_(other.p_) {}
        ConstIterator(const ConstIterator& other): p_(other.p_) {}
        const ConstIterator& operator=(const ConstIterator& other) {p_ = other.p_; return *this;}
        const ConstIterator& operator=(const Iterator& other) {p_ = other.p_; return *this;}

        ConstIterator& operator++()    {p_++; return *this;} // prefix++
        ConstIterator  operator++(int) {ConstIterator tmp(*this); ++(*this); return tmp;} // postfix++
        ConstIterator& operator--()    {p_--; return *this;} // prefix--
        ConstIterator  operator--(int) {ConstIterator tmp(*this); --(*this); return tmp;} // postfix--

        void           operator+=(const int& n) {p_ += n;}
        //void           operator+=(const ConstIterator& other) {p_ += other.p_ ;}
        ConstIterator  operator+ (const int& n) const {ConstIterator tmp(*this); tmp += n; return tmp;}
        //ConstIterator  operator+ (const ConstIterator& other) const {ConstIterator tmp(*this); tmp += other; return tmp;}

        void           operator-=(const int& n) {p_ -= n;}
        //void           operator-=(const ConstIterator& other) {p_ -= other.p_;}
        ConstIterator  operator- (const int& n) const {ConstIterator tmp(*this); tmp -= n; return tmp;}
        int            operator- (const ConstIterator& other) const {return p_ - other.p_;}

        bool operator< (const ConstIterator& other) const {return (p_-other.p_)< 0;}
        bool operator<=(const ConstIterator& other) const {return (p_-other.p_)<=0;}
        bool operator> (const ConstIterator& other) const {return (p_-other.p_)> 0;}
        bool operator>=(const ConstIterator& other) const {return (p_-other.p_)>=0;}
        bool operator==(const ConstIterator& other) const {return  p_ == other.p_; }
        bool operator!=(const ConstIterator& other) const {return  p_ != other.p_; }

        const unsigned char& operator[](const int& n) const {return *(p_+n);}
        const unsigned char& operator*()  const {return *p_;}
        const unsigned char* operator->() const {return  p_;}

    private:
        const unsigned char* p_;
    };
   
    /**
     * Constructor 
     */
    //Frame();
    
    /**
     * Constructor.
     * Create an empty frame object not initialized with length.
     * On the other hand are initialized parameters subsequently defined.
     * If sizeFCS > 7, will instantiate frame with default parameter.
     * \param header
     * \param autoFCS
     * \param sizeFCS
     */
    Frame(bool header = true, bool autoFCS = true, int sizeFCS = 2);
     
    /**
     * Constructor.
     * Create an empty frame object with the fields specified by parameter.
     * If 'autoFCS' parameter is true, FCS field is not present because 
     * automatically added by transceiver. Dimension of FCS is included in LEN 
     * (2 byte).
     * Note: if set autoFCS then sizeFCS is not considered.
     * \param sizePayload dimension of payload;
     * \param header if header is present or not;
     * \param autoFCS if true no FCS field is reserved;
     * \param sizeFCS size of FCS field < 8
     */
    Frame(int sizePayload, bool header = true, bool autoFCS = true, int sizeFCS = 2);
    
    /**
     * Constructor.
     * Create a filled frame object whit LEN field, payload field.
     * FCS field is not present because automatically added by transceiver.
     * Dimension of FCS is included in LEN (2 byte).
     * \param sizePayload
     * \param ppaylod
     */
    Frame(int sizePayload, const unsigned char* ppayload);
    
    /**
     * 
     * \param frame
     */
    Frame(const Frame& frame);
    
    bool operator==(const Frame& frame) const;
    
    bool operator!=(const Frame& frame) const;
    
    const Frame& operator=(const Frame& other);
    
    //const unsigned char& operator[](int idx) const;
    
    //unsigned char& operator[](int& idx);
   
    
    /**
     * \return the size of frame, if size is negative than frame is not initialize
     */
    int getFrameSize() const;

    /**
     * \return an iterator that points to the byte following the length field
     */
    Iterator begin();
    
    /**
     * \return  an iterator that points to the end of the frame
     */
    Iterator end();
    
    /**
     * \return  a constant iterator that points to the start of the frame
     */
    ConstIterator begin() const;
    
    ConstIterator begin_() const;
    
    /**
     * \return  a constant iterator that points to the end of the frame
     */
    ConstIterator end() const;
    
    /**
     * Initialize the frame with size contained in LEN fiedl parameter.
     * SizeLEN must also include the length of the FCS field although this is set to auto.
     * Automatically write field LEN.
     * \param sizeLEN
     * \return true if init is successful false otherwise
     */
    bool initFrame(int sizeLEN);
    
    /**
     * 
     * \param len return the content of the field LEN
     * \return true if no error false otherwise
     */
    int getLEN() const;

    /**
     * 
     * \param fcf
     */
    bool setFCF(const unsigned char* fcf);
    
    /**
     * \param fcf
     */
    bool getFCF(unsigned char* fcf) const;
    
    /**
     * \param seq
     */
    bool setSEQ(unsigned char seq);
    
    /**
     * 
     * \param seq
     * \return 
     */
    bool getSEQ(unsigned char& seq) const;
    
    /**
     * 
     * \param sourcePanId
     * \return 
     */
    bool setSourcePanId(const unsigned char* sourcePanId) ;
    
    /**
     * 
     * \param sourcePanId
     * \return 
     */
    bool getSourcePanId(unsigned char* sourcePanId) const;
    
    /**
     * 
     * \param sourceShrortAddr
     * \return 
     */
    bool setSourceShortAddr(const unsigned char* sourceShrortAddr);

    /**
     * 
     * \param sourceShrortAddr
     * \return 
     */
    bool getSourceShortAddr(unsigned char* sourceShrortAddr) const;
    
    /**
     * 
     * \param destPanId
     * \return 
     */
    bool setDestPanId(const unsigned char* destPanId);
    
    /**
     * 
     * \param destPanId
     * \return 
     */
    bool getDestPanId(unsigned char* destPanId) const;
    
    /**
     * 
     * \param destShrortAddr
     * \return 
     */
    bool setDestShortAddr(const unsigned char* destShrortAddr);
    
    /**
     * 
     * \param destShrortAddr
     * \return 
     */
    bool getDestShortAddr(unsigned char* destShrortAddr) const;
    
    /**
     * 
     * \param header
     * \return 
     */
    bool setHeader(const unsigned char* header);
    
    /**
     * 
     * \param header
     * \return 
     */
    bool getHeader(unsigned char* header) const; 
    
    /**
     * 
     * \return 
     */
    bool haveHeader() const;
    
    /**
     * \return the payload of frame 
     */
    bool setPayload(const unsigned char* payload);
    
    /**
     * 
     * \param payload
     * \return 
     */
    bool getPayload(unsigned char* payload) const;
    
    /**
     * 
     * \return 
     */
    int getSizePayload() const;
    
    /**
     * 
     * \return 
     */
    bool isAutoFCS() const;
    
    /**
     * 
     * \return 
     */
    int getSizeFCS() const;
    
    /**
     * Allows you to write your fcs field.
     * \param FCS fcs field
     * \return 
     */
    bool setFCS(const unsigned char* fcs);
    
    /**
     * 
     * \param fcs
     * \return 
     */
    bool getFCS(unsigned char* fcs) const;
    
    /**
     * \return true if initialization is wrong false otherwise 
     */
    bool isNotInit() const;
    
    
    ~Frame();
private:

    struct typeFrame{
        unsigned char autoFcs : 1;
        unsigned char fcsLen : 3;
        unsigned char header : 1;
        unsigned char nothing : 3;

        typeFrame(bool autoFcs =1, int fcsLen=2, bool header=1) : autoFcs(autoFcs), fcsLen(fcsLen), header (header)
        {
            /**
            this->autoFcs = autoFcs;
            this->fcsLen = fcsLen;
            this->header = header;
             */
        }
        
        typeFrame operator=(const typeFrame& a)
        {
            autoFcs = a.autoFcs;
            fcsLen = a.fcsLen;
            header = a.header;
            return *this;
        }
                
        bool operator==(const typeFrame& a) const
        {
            if (autoFcs == a.autoFcs &&
                fcsLen == a.fcsLen &&
                header == a.header ) return true;
            else
                return false;
        }
        
        bool operator!=(const typeFrame& a) const
        {
            if (autoFcs == a.autoFcs &&
                fcsLen == a.fcsLen &&
                header == a.header ) return false;
            else
                return true;
        }
    };
    
    unsigned char* pframe;
    typeFrame ctrlFrame;
    
};


#endif	/* FRAME_H */

