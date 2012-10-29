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

#ifndef NRF24L01_H
#define	NRF24L01_H

/**
 * This class allows to interface with the nRF24L01+
 * Access to this class is not mutex protected to guard concurrent access,
 * as it is assumed that there exists a thread dedicated to the radio.
 */
class Nrf24l01
{
public:
    /**
     * Operating mode of the nRF24L01+
     */
    enum Mode
    {
        TX,
        RX,
        SLEEP
    };
    
    /**
     * \return an instance to this class (singleton)
     */
    static Nrf24l01& instance();
    
    /** 
     * \param mode the new nRF operating mode
     */
    void setMode(Mode mode);
    
    /**
     * \param freq the new nRF operating frequency, in MHz, from 2400 to 2500
     */
    void setFrequency(unsigned short freq);
    
    /**
     * Set the packet length used both for transmitting and receiving.
     * The transceiver sends packet of that length and ingores received packets
     * of different length.
     * \param length packet length, must be between 1 and 32, default is 16.
     */
    void setPacketLength(unsigned short length);
    
    /**
     * \return the packet length 
     */
    unsigned short getPacketLength() const { return length; }
    
    /**
     * Set the device's addresss. It is used both in transmission and reception
     * \param address device address
     */
    void setAddress(const unsigned char address[3]);
    
    /**
     * \return true if the nrf IRQ signal of the NRF24L01+ is asserted 
     */
    bool irq() const;
    
    /**
     * Write one packet. The nRF must be configured in TX mode and
     * isTxSlotAvailable() should return true, signaling that the TX fifo has
     * room for the packet. This function returns immediately, before the packet
     * has been sent. To know when the packet has been sent monitor the irq()
     * signal and allPacketsSent(), and lastly call endWritePacket().
     * 
     * \code
     * //Sending a single packet
     * char packet[packetSize];
     * fillPacket(packet);
     * nrf.writePacket(packet);
     * while(nrf.irq()==false) ; //Wait
     * nrf.endWritePacket();
     * 
     * //Sending multiple back to back packets
     * char packets[numpackets][packetSize];
     * fillpackets(packets,numpackets);
     * for(int i=0;i<numpackets;)
     * {
     *     nrf.clearTxIrq();
     *     if(nrf.isTxSlotAvailable()) nrf.writePacket(packets[i++]);
     *     else while(nrf.irq()==false) ; //Wait
     * }
     * for(;;)
     * {
     *     nrf.clearTxIrq();
     *     if(nrf.allPacketsSent()) break;
     *     while(nrf.irq()==false) ; //Wait
     * }
     * nrf.endWritePacket();
     * \endcode
     * 
     * \param packet packet to send. The packet length is the one set through
     * setPacketLength()
     */
    void writePacket(const void *packet);
    
    /**
     * \return true if a transmission slot is available in the TX fifo 
     */
    bool isTxSlotAvailable() const;
    
    /**
     * \return true if all packets in the TX fifo have been sent 
     */
    bool allPacketsSent() const;
    
    /**
     * Clears the IRQ signal that is set after a packet is transmitted
     */
    void clearTxIrq();
    
    /**
     * Resets the value returned by irq() after a packet has been sent 
     */
    void endWritePacket();

    /**
     * Read a packet. The nRF must be configured in RX mode, and
     * isRxPacketAvailable() must be returning true signaling that a packet
     * has been received.
     * 
     * \code
     * while(nrf.irq()==false) ;
     * //If IRQ signaled, one or more packets are in the RX fifo
     * 
     * while(nrf.isRxPacketAvailable())
     * {
     *     char packet[packetSize];
     *     nrf.readPacket(packet);
     *     //Process packet
     * }
     * \endcode
     * 
     * \param packet packet to send. The packet length is the one set through
     * setPacketLength()
     */
    void readPacket(void *packet);

    /**
     * After setting the mode to RX the radio is kept ready to receive but in
     * low power state. By calling this function the radio can actually start
     * receiving.
     */
    void startReceiving();
    
    /**
     * \return true if a packet is available to be read from the RX fifo 
     */
    bool isRxPacketAvailable() const;
    
private:
    /**
     * Constructor
     */
    Nrf24l01();
    
    Nrf24l01(const Nrf24l01&);
    Nrf24l01& operator=(const Nrf24l01&);
    
    /**
     * List of most of the nRF24L01+ registers.
     */
    enum NrfReg
    {
        CONFIG=0x0,
        EN_AA=0x1,
        EN_RXADDR=0x2,
        SETUP_AW=0x3,
        SETUP_RETR=0x4,
        RF_CH=0x5,
        RF_SETUP=0x6,
        STATUS=0x7,
        OBSERVE_TX=0x8,
        RPD=0x9,
        RX_ADDR=0xa,
        TX_ADDR=0x10,
        RX_PW_P0=0x11,
        FIFO_STATUS=0x17,
        DYNPD=0x1c,
        FEATURE=0x1d
    };
    
    /**
     * List of some bits in the STATUS register
     */
    enum StatusBits
    {
        RX_DR=0x40, // 1 if there's data in the RX fifo (IRQ pulled low if 1)
        TX_DS=0x20, // 1 if a packet has been transmitted (IRQ pulled low if 1)
        TX_FULL=1   // 1 if TX fifo full
    };
    
    /**
     * Write into one of the nRF24L01+ single byte registers
     * \param reg selected register
     * \param data data to write
     */
    void writeReg(NrfReg reg, unsigned char data);
    
    /**
     * Write into one of the nRF24L01+ multi byte registers
     * \param reg selected register
     * \param data data to write
     * \param len data length
     */
    void writeReg(NrfReg reg, const unsigned char *data, int len);
    
    /**
     * \return the STATUS register of the nRF
     */
    unsigned char readStatus() const;
    
    unsigned short frequency; ///< Operating frequency
    unsigned short length; ///< Packet length
    Mode mode; ///< Operating mode
    unsigned char address[3]; //< Device address
};

#endif //NRF24L01_H
