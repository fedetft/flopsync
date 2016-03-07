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

#ifndef CC2520_H
#define	CC2520_H
#include "timer.h"
#include <miosix.h>

#define CC2520_DEBUG 1 // 0 no debug; 1 soft debub; 2 pedantic debug; 4 for test;

#if CC2520_DEBUG >0
#include "../board_setup.h"
#include <cstdio>
#endif //CC2520_DEBUG




#include "frame.h"
/**
 * This class allows to interface with the cc2520
 * Access to this class is not mutex protected to guard concurrent access,
 * as it is assumed that there exists a thread dedicated to the radio.
 */
class Cc2520
{
public:
    
    /**
     * \return a singletone instance of transceiver cc2520
     */
    static Cc2520& instance();
    
    /**
     * Operating mode of the transceiver
     */
    enum Mode {
        TX,
        RX,
        SLEEP,
        IDLE,
        DEEP_SLEEP
    };
    
    /**
     * Nominal transmit power
     */
    enum Power {
        P_5 = 0xf7,  ///< +5dBm
        P_3 = 0xf2,  ///< +3dBm
        P_2 = 0xab,  ///< +2dBm
        P_1 = 0x13,  ///< +1dBm
        P_0 = 0x32,  ///< +0dBm
        P_m2 = 0x81, ///< -2dBm
        P_m7 = 0x2c, ///< -7dBm
        P_m18 = 0x03 ///< -18dBm
    };
    
    /**
     * \param mode the new cc2520 operating mode
     */
    void setMode(Mode mode);
    
    /**
     * Set transmit power
     * \param power transmit power
     */
    void setTxPower(Power power);
    
    /**
     * \param freq the new cc2520 operating frequency, in MHz, from 2394 to 2507
     */
    void setFrequency(unsigned short freq);
   
    /**
     * \param channel the new Transceiver operating frequency channel.
     * There are 16 channel from 11 to 26.
     */
    void setFrequencyChannel(unsigned char channel);
    
    /**
     * Set the device's short addresss. It is used both in transmission and 
     * reception when frame filtering is active.
     * \param address device address
     */
    void setShortAddress(const unsigned char address[2]);
    
    /**
     * Set the Privata Area Network ID (PAN ID). It is used both in transmission
     * and reception when frame filtering is active.
     * \param panId private area network identifier
     */
    void setPanId(const unsigned char panId[2]);
    
    /**
     * Write in TX fifo buffer one frame. 
     * The first byte of frame is automatically witten and it
     * indicates the length of the frame (excluding it) and
     * including the two bytes for the FCS field. 
     * Of the first byte, Frame LEN, uses only the first 7 bits 
     * FCS is automatically written by writeFrame.
     * (frame length up to 127 bytes)
     * \param pframe
     * \return integer code status number:
     *          -3 error not in TX state
     *          -2 invalid pframe parameter, pframe is null;
     *          -1 invalid length parameter;
     *           0 write successful;
     *           1 exception tx buffer overflow;
     */
    int writeFrame(unsigned char length, const unsigned char* pframe);
    
    /**
     * Write in TX fifo buffer one frame. 
     * \param frame object Frame not initialized
     * \return  integer code status number:
     *          -3 error: not in TX state
     *          -1 invalid frame parameter, probably frame is Init already;
     *           0 write successful;
     *           1 exception tx buffer overflow;
     */   
    int writeFrame(const Frame& frame);
    
    /**
     * Reads from Rx FIFO buffer on the first frame received. The maximum length
     * of a frame is 127 bytes (includes the 2-byte FCS that are not returned).
     * \param length is an 'in/out' parameter. In 'in', indicates the size of
     * pframe (the length of vector chars), in 'out' returns the real number of 
     * chars stored in pframe.
     * \param pframe frame pointer to RX FIFO frame. The frame returned not 
     * contain LEN and FCS. 
     * \return integer code status number:
     *       -3 error: not in RX state
     *       -2 invalid pframe parameter, pframe is null;
     *       -1 invalid length parameter;
     *        0 the size of pframe is lower than that of the read frame in 
     *          the buffer (only lenght bytes are returned, the rest are lost)
     *        1 the size of pFrame is greater than or equal to that one read 
     *          in the frame buffer. The length parameter is updated to the 
     *          actual size of pFrame;
     *        2 exception in read buffer operation: BUFFER_UNDERFLOW;
     *        3 FCS doesn't match; the frame contents is still returned;
     */
    int readFrame(unsigned char& lenght, unsigned char* pframe)const;
    
    /**
     * Reads from Rx FIFO buffer on the first frame received. The maximum length
     * of a frame is 128 bytes.
     * \param frame object Frame not initialized
     * \return integer code status number:
     *          -3 error not in RX state
     *          -1 invalid frame parameter, probably frame is Init already;
     *           0 write successful;
     *           1 exception rx buffer underflow;
     *           2 FCS doesn't match; the frame contents is still returned;
     */          
    int readFrame(Frame& frame) const;
    
    /**
     * Flush TX FIFO buffer
     * \return integer code status number:
     *          -1 error not in TX state
     *           1 buffer flushed
     */
    int flushTxFifoBuffer() const;
    
    /**
     * Flush RX FIFO buffer
     * \return integer code status number:
     *          -1 error not in RX state 
     *           1 buffer flushed
     */
    int flushRxFifoBuffer() const;
    
    /**
     * Send the frame contents in TX FIFO buffer
     * \return integer code status number:
     *          -1 error not in TX state 
     *           0 exception raised frame aborted
     *           1 frame send
     */
    int sendTxFifoFrame() const;
    
    /**
     * When called if raised, disable SFD exception.
     * \return integer code status number:
     *          -1 error not in TX state 
     *           0 frame transmission not completed
     *           1 frame transmission completed
     */
    int isSFDRaised() const;
    
    /**
     * When called if raised, disable TX_FRM_DONE exception.
     * \return integer code status number:
     *          -1 error not in TX state 
     *           0 frame transmission not completed
     *           1 frame transmission completed
     */
    int isTxFrameDone() const;
    
    /**
     *  When called if raised, disable RX_FRM_DONE exception.
     * \return integer code status number:
     *          -1 error not in RX state 
     *           0 no frame available
     *           1 frame is available to be read from the RX FIFO
     */
    int isRxFrameDone() const;   
    
    /**
     * \return integer code status number:
     *          -1 error not in RX state 
     *           0 RX FIFO buffer is empty
     *           1 at least one byte is stored in RX FIFO buffer
     */
    int isRxBufferNotEmpty() const;
    
    /**
     * \return the mode of transceiver:
     *          TX,RX,SLEEP,DEEP_SLEEP
     */
    Mode getMode() const;
    
    /**
     * 
     * @param fcs
     * @return 
     */
    void setAutoFCS(bool fcs);
    
    /**
     * Start rx/tx calibration, this operation take 192us about
     * \return integer code status number:
     *          -1 error not in TX state 
     *           0 exception raised frame aborted
     *           1 calibration done
     */ 
    int stxcal();
    
    int getRssi() const
    {
        int rawRssi=(rssiData>>8) & 0xff;
        if(rawRssi & 0x80) rawRssi-=256; //Convert 8 bit 2's complement to int
        return rawRssi-76; //See datasheet
    }
    
    
private:
    /**
     * Constructor
     */
    Cc2520();
    
    Cc2520(const Cc2520&);
    Cc2520& operator=(const Cc2520&);
    
    /**
     * List of Registers of CC2520
     */
    enum Cc2520FREG
    {
        // FREG Registers
        CC2520_FRMFILT0         = 0x00,
        CC2520_FRMFILT1         = 0x01,
        CC2520_SRCMATCH         = 0x02,
        CC2520_SRCSHORTEN0      = 0x04,
        CC2520_SRCSHORTEN1      = 0x05,
        CC2520_SRCSHORTEN2      = 0x06,
        CC2520_SRCEXTEN0        = 0x08,
        CC2520_SRCEXTEN1        = 0x09,
        CC2520_SRCEXTEN2        = 0x0A,
        CC2520_FRMCTRL0         = 0x0C,
        CC2520_FRMCTRL1         = 0x0D,
        CC2520_RXENABLE0        = 0x0E,
        CC2520_RXENABLE1        = 0x0F,
        CC2520_EXCFLAG0         = 0x10,
        CC2520_EXCFLAG1         = 0x11,
        CC2520_EXCFLAG2         = 0x12,
        CC2520_EXCMASKA0        = 0x14,
        CC2520_EXCMASKA1        = 0x15,
        CC2520_EXCMASKA2        = 0x16,
        CC2520_EXCMASKB0        = 0x18,
        CC2520_EXCMASKB1        = 0x19,
        CC2520_EXCMASKB2        = 0x1A,
        CC2520_EXCBINDX0        = 0x1C,
        CC2520_EXCBINDX1        = 0x1D,
        CC2520_EXCBINDY0        = 0x1E,
        CC2520_EXCBINDY1        = 0x1F,
        CC2520_GPIOCTRL0        = 0x20,
        CC2520_GPIOCTRL1        = 0x21,
        CC2520_GPIOCTRL2        = 0x22,
        CC2520_GPIOCTRL3        = 0x23,
        CC2520_GPIOCTRL4        = 0x24,
        CC2520_GPIOCTRL5        = 0x25,
        CC2520_GPIOPOLARITY     = 0x26,
        CC2520_GPIOCTRL         = 0x28,
        CC2520_DPUCON           = 0x2A,
        CC2520_DPUSTAT          = 0x2C,
        CC2520_FREQCTRL         = 0x2E,
        CC2520_FREQTUNE         = 0x2F,
        CC2520_TXPOWER          = 0x30,
        CC2520_TXCTRL           = 0x31,
        CC2520_FSMSTAT0         = 0x32,
        CC2520_FSMSTAT1         = 0x33,
        CC2520_FIFOPCTRL        = 0x34,
        CC2520_FSMCTRL          = 0x35,
        CC2520_CCACTRL0         = 0x36,
        CC2520_CCACTRL1         = 0x37,
        CC2520_RSSI             = 0x38,
        CC2520_RSSISTAT         = 0x39,
        CC2520_RXFIRST          = 0x3C,
        CC2520_RXFIFOCNT        = 0x3E,
        CC2520_TXFIFOCNT        = 0x3F,
    };
    
    enum Cc2520SREG
    {
        // SREG registers
        CC2520_CHIPID           = 0x40,
        CC2520_VERSION          = 0x42,
        CC2520_EXTCLOCK         = 0x44,
        CC2520_MDMCTRL0         = 0x46,
        CC2520_MDMCTRL1         = 0x47,
        CC2520_FREQEST          = 0x48,
        CC2520_RXCTRL           = 0x4A,
        CC2520_FSCTRL           = 0x4C,
        CC2520_FSCAL0           = 0x4E,
        CC2520_FSCAL1           = 0x4F,
        CC2520_FSCAL2           = 0x50,
        CC2520_FSCAL3           = 0x51,
        CC2520_AGCCTRL0         = 0x52,
        CC2520_AGCCTRL1         = 0x53,
        CC2520_AGCCTRL2         = 0x54,
        CC2520_AGCCTRL3         = 0x55,
        CC2520_ADCTEST0         = 0x56,
        CC2520_ADCTEST1         = 0x57,
        CC2520_ADCTEST2         = 0x58,
        CC2520_MDMTEST0         = 0x5A,
        CC2520_MDMTEST1         = 0x5B,
        CC2520_DACTEST0         = 0x5C,
        CC2520_DACTEST1         = 0x5D,
        CC2520_ATEST            = 0x5E,
        CC2520_DACTEST2         = 0x5F,
        CC2520_PTEST0           = 0x60,
        CC2520_PTEST1           = 0x61,
        CC2520_RESERVED         = 0x62,
        CC2520_DPUBIST          = 0x7A,
        CC2520_ACTBIST          = 0x7C,
        CC2520_RAMBIST          = 0x7E,
    };

    enum Cc2520Mem
    {
        CC2520_MEM_ADDR_BASE    = 0x3EA
    };
    
    /**
     * List of Instruction Set Architecture of CC2520
     */
    enum Cc2520SpiIsa 
    {
        CC2520_INS_SNOP                 = 0x00, 
        CC2520_INS_IBUFLD               = 0x02, 
        CC2520_INS_SIBUFEX              = 0x03, //Command Strobe
        CC2520_INS_SSAMPLECCA           = 0x04, //Command Strobe
        CC2520_INS_SRES                 = 0x0f, 
        CC2520_INS_MEMORY_MASK          = 0x0f, 
        CC2520_INS_MEMORY_READ          = 0x10, // MEMRD
        CC2520_INS_MEMORY_WRITE         = 0x20, // MEMWR
        CC2520_INS_RXBUF                = 0x30, 
        CC2520_INS_RXBUFCP              = 0x38, 
        CC2520_INS_RXBUFMOV             = 0x32, 
        CC2520_INS_TXBUF                = 0x3A, 
        CC2520_INS_TXBUFCP              = 0x3E, 
        CC2520_INS_RANDOM               = 0x3C, 
        CC2520_INS_SXOSCON              = 0x40, 
        CC2520_INS_STXCAL               = 0x41, //Command Strobe
        CC2520_INS_SRXON                = 0x42, //Command Strobe
        CC2520_INS_STXON                = 0x43, //Command Strobe
        CC2520_INS_STXONCCA             = 0x44, //Command Strobe
        CC2520_INS_SRFOFF               = 0x45, //Command Strobe
        CC2520_INS_SXOSCOFF             = 0x46, //Command Strobe
        CC2520_INS_SFLUSHRX             = 0x47, //Command Strobe
        CC2520_INS_SFLUSHTX             = 0x48, //Command Strobe
        CC2520_INS_SACK                 = 0x49, //Command Strobe
        CC2520_INS_SACKPEND             = 0x4A, //Command Strobe
        CC2520_INS_SNACK                = 0x4B, //Command Strobe
        CC2520_INS_SRXMASKBITSET        = 0x4C, //Command Strobe
        CC2520_INS_SRXMASKBITCLR        = 0x4D, //Command Strobe
        CC2520_INS_RXMASKAND            = 0x4E, 
        CC2520_INS_RXMASKOR             = 0x4F,
        CC2520_INS_MEMCP                = 0x50,         
        CC2520_INS_MEMCPR               = 0x52, 
        CC2520_INS_MEMXCP               = 0x54, 
        CC2520_INS_MEMXWR               = 0x56, 
        CC2520_INS_BCLR                 = 0x58, 
        CC2520_INS_BSET                 = 0x59, 
        CC2520_INS_CTR_UCTR             = 0x60,
        CC2520_INS_CBCMAC               = 0x64, 
        CC2520_INS_UCBCMAC              = 0x66, 
        CC2520_INS_CCM                  = 0x68, 
        CC2520_INS_UCCM                 = 0x6A,
        CC2520_INS_ECB                  = 0x70, 
        CC2520_INS_ECBO                 = 0x72,
        CC2520_INS_ECBX                 = 0x74,
        CC2520_INS_INC                  = 0x78, 
        CC2520_INS_ABORT                = 0x7F, 
        CC2520_INS_REGISTER_READ        = 0x80, 
        CC2520_INS_REGISTER_WRITE       = 0xC0, 
    };

    /**
     * List of some bits in the STATUS register
     */
    enum Cc2520StatusBits
    {
        CC2520_STATUS_RX_A    = 0x01,  // 1 if RX is active
        CC2520_STATUS_TX_A    = 0x02,  // 1 if TX is active
        CC2520_STATUS_DPU_L_A = 0x04,  // 1 if low  priority DPU is active
        CC2520_STATUS_DPU_H_A = 0x08,  // 1 if high  priority DPU is active
        CC2520_STATUS_EXC_B   = 0x10,  // 1 if at least one exception on channel B is raise
        CC2520_STATUS_EXC_A   = 0x20,  // 1 if at least one exception on channel A is raise
        CC2520_STATUS_RSSI_V  = 0x40,  // 1 if RSSI value is valid
        CC2520_STATUS_XOSC    = 0x80   // 1 if XOSC is stable and running
    };

    /**
     * List of exception: register excflag0
     */
    enum Cc2520Exc0
    {
        CC2520_EXC_RF_IDLE           = 0x01,  
        CC2520_EXC_TX_FRM_DONE       = 0x02,  
        CC2520_EXC_TX_ACK_DONE       = 0x04,  
        CC2520_EXC_TX_UNDERFLOW      = 0x08,  
        CC2520_EXC_TX_OVERFLOW       = 0x10,  
        CC2520_EXC_RX_UNDERFLOW      = 0x20,  
        CC2520_EXC_RX_OVERFLOW       = 0x40,  
        CC2520_EXC_RXENABLE_ZERO     = 0x80
    };
    /**
     * List of exception: register excflag1
     */
    enum Cc2520Exc1
    {
        CC2520_EXC_RX_FRM_DONE       = 0x01,
        CC2520_EXC_RX_FRM_ACCEPETED  = 0x02,  
        CC2520_EXC_SR_MATCH_DONE     = 0x04,  
        CC2520_EXC_SR_MATCH_FOUND    = 0x08, 
        CC2520_EXC_FIFOP             = 0x10,
        CC2520_EXC_SFD               = 0x20,
        CC2520_EXC_DPU_DONE_L        = 0x40, 
        CC2520_EXC_DPU_DONE_H        = 0x80
    };
    /**
     * List of exception: register excflag2
     */
    enum Cc2520Exc2
    {
        CC2520_EXC_MEMADDR_ERROR     = 0x01, 
        CC2520_EXC_USAGE_ERROR       = 0x02,
        CC2520_EXC_OPERAND_ERROR     = 0x04, 
        CC2520_EXC_SPI_ERROR         = 0x08, 
        CC2520_EXC_RF_NO_CLOCK       = 0x10, 
        CC2520_EXC_RX_FRM_ABORTED    = 0x20,
        CC2520_EXC_RXBUFMOV_TIMEOUT  = 0x40
    };
     
    /**
     * Write into one of the cc2520 single byte fast registers
     * \param reg selected register
     * \param data data to write
     */
    bool writeReg(Cc2520FREG reg, unsigned char data) const;

    /**
     * Write into one of the cc2520 multi byte fast registers
     * \param reg selected register
     * \param data data to write
     * \param len data length
     */
    bool writeReg(Cc2520FREG reg, const unsigned char *data, int len) const;
    
        /**
     * Write into one of the cc2520 single byte slow registers
     * \param reg selected register
     * \param data data to write
     */
    bool writeReg(Cc2520SREG reg, unsigned char data) const;

    /**
     * Write into one of the cc2520 multi byte slow registers
     * \param reg selected register
     * \param data data to write
     * \param len data length
     */
    bool writeReg(Cc2520SREG reg, const unsigned char *data, int len) const;

    /**
     * Read one of the cc2520 single byte fast registers
     * \param reg selected register
     * \return the value of register
     * \return true if the operation is successful, false otherwise
     */
    bool readReg(Cc2520FREG reg, unsigned char& result) const;
    
    /**
     * Read one of the cc2520 single byte fast registers
     * \param reg selected register
     * \return the value of register
     * \return true if the operation is successful, false otherwise
     */
    bool readReg(Cc2520SREG reg, unsigned char& result) const;
    
    /**
     * \return the STATUS register of the nRF
     */
    unsigned char readStatus() const;
    
    /**
     * This function allows you to perform a byte instructions that have 
     * no parameters.
     * \param ins instruction command
     * \return status byte
     */
    unsigned char commandStrobe(Cc2520SpiIsa ins) const;
    
    /**
     * Return true if exc in EXCFLAG0 register, is raised and reset it,
     * false otherwise.
     * \param exc exception to match
     * \param status status byte
     * \return true if exception is raised false otherwise
     */
    bool isExcRaised(Cc2520Exc0 exc, 
                     unsigned char status = CC2520_STATUS_EXC_A) const;
    
    /**
     * Return true if exc in EXCFLAG1 register, is raised and reset it,
     * false otherwise.
     * \param exc exception to match
     * \param status status byte
     * \return true if exception is raised false otherwise
     */
    bool isExcRaised(Cc2520Exc1 exc, 
                     unsigned char status = CC2520_STATUS_EXC_A) const;
    
    /**
     * Return true if exc in EXCFLAG2 register, is raised and reset it,
     * false otherwise.
     * \param exc exception to match
     * \param status status byte
     * \return true if exception is raised false otherwise
     */
    bool isExcRaised(Cc2520Exc2 exc, 
                     unsigned char status = CC2520_STATUS_EXC_A ) const;
    
    /**
     * Write initial configuration Transceiver, or wake-up deep-sleep
     */
    void initConfigureReg();

    //delay for cs spi (2 tick ~80ns)
    inline void delay() const
    {
        // This delay has been calibrated to take x microseconds
        // It is written in assembler to be independent on compiler optimization
        asm volatile("           nop    \n"
                     "           nop    \n");
    }
    
    void wait();
    
    unsigned short frequency;
    unsigned char panId[2];
    unsigned char shortAddress[2];
    mutable unsigned short rssiData; //RSSI data of last rx operation, if autoFCS==true
    bool autoFCS;
    Power power;
    Mode mode; //< Operating mode
    Timer& timer;
};

#endif //CC2520_H
