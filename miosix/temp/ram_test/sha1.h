/*
 *  sha1.h
 *
 *  Copyright (C) 1998, 2009
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved.
 *
 * Freeware Public License (FPL)
 *
 * This software is licensed as "freeware."  Permission to distribute
 * this software in source and binary forms, including incorporation
 * into other products, is hereby granted without a fee.  THIS SOFTWARE
 * IS PROVIDED 'AS IS' AND WITHOUT ANY EXPRESSED OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE AUTHOR SHALL NOT BE HELD
 * LIABLE FOR ANY DAMAGES RESULTING FROM THE USE OF THIS SOFTWARE, EITHER
 * DIRECTLY OR INDIRECTLY, INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA
 * OR DATA BEING RENDERED INACCURATE.
 *
 *****************************************************************************
 *  $Id: sha1.h 12 2009-06-22 19:34:25Z paulej $
 *****************************************************************************
 *
 *  Description:
 *      This class implements the Secure Hashing Standard as defined
 *      in FIPS PUB 180-1 published April 17, 1995.
 *
 *      Many of the variable names in this class, especially the single
 *      character names, were used because those were the names used
 *      in the publication.
 *
 *      Please read the file sha1.cpp for more information.
 *
 */

#ifndef _SHA1_H_
#define _SHA1_H_

class SHA1
{

    public:

        SHA1();
        virtual ~SHA1();

        /*
         *  Re-initialize the class
         */
        void Reset();

        /*
         *  Returns the message digest
         */
        bool Result(unsigned *message_digest_array);

        /*
         *  Provide input to SHA1
         */
        void Input( const unsigned char *message_array,
                    unsigned            length);
        void Input( const char  *message_array,
                    unsigned    length);
        void Input(unsigned char message_element);
        void Input(char message_element);
        SHA1& operator<<(const char *message_array);
        SHA1& operator<<(const unsigned char *message_array);
        SHA1& operator<<(const char message_element);
        SHA1& operator<<(const unsigned char message_element);

    private:

        /*
         *  Process the next 512 bits of the message
         */
        void ProcessMessageBlock();

        /*
         *  Pads the current message block to 512 bits
         */
        void PadMessage();

        /*
         *  Performs a circular left shift operation
         */
        inline unsigned CircularShift(int bits, unsigned word);

        unsigned H[5];                      // Message digest buffers

        unsigned Length_Low;                // Message length in bits
        unsigned Length_High;               // Message length in bits

        unsigned char Message_Block[64];    // 512-bit message blocks
        int Message_Block_Index;            // Index into message block array

        bool Computed;                      // Is the digest computed?
        bool Corrupted;                     // Is the message digest corruped?
    
};

// Added by TFT -- begin
#include <string>
#include <cstdio>

const int sha1size=20;

inline std::string sha1(const std::string& message)
{
	SHA1 hash;
	hash.Input(message.c_str(),message.length());
	unsigned int r[sha1size/4];
	hash.Result(r);
	char resultstr[2*sha1size+1];
	std::sprintf(resultstr,"%04x%04x%04x%04x%04x",r[0],r[1],r[2],r[3],r[4]);
	return std::string(resultstr);
}

inline void sha1binary(const std::string& message, unsigned char digest[sha1size])
{
	SHA1 hash;
	hash.Input(message.c_str(),message.length());
	unsigned int r[sha1size/4];
	hash.Result(r);
	for(int i=0;i<sha1size;i++) digest[i]=(r[i/4] >> 8*(3-i%4)) & 0xff;
}
// Added by TFT -- end

#endif
