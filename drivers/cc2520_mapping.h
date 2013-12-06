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

#ifndef CC2520_MAP_H
#define	CC2520_MAP_H

#include "transceiver.h"
#include "cc2520.h"
#include "../test_cc2520/test_config.h"

/**
 * This class allows to interface with the cc2520
 * Access to this class is not mutex protected to guard concurrent access,
 * as it is assumed that there exists a thread dedicated to the radio.
 */
class Cc2520Mapping : public Transceiver
{
public:
    /**
     * \param mode the new cc2520 operating mode
     */
    void setMode(Mode mode);
    
    /**
     * \param freq the new cc2520 operating frequency, in MHz, from 2394 to 2507
     */
    void setFrequency(unsigned short freq);

    /**
     * Set the packet length used both for transmitting and receiving.
     * The transceiver sends packet of that length and ignores received packets
     * of different length.
     * \param length packet length, must be between 1 and 127-2(FCS), default is 16.
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
     * \return true if:
     *           CC2520 is in TX mode and one packet has been transmitted
     *           CC2520 is in RX mode and one byte is in RX buffer
     *         false otherwise.
     */
    bool irq() const;

    /**
     * Write one packet. The cc2520 must be configured in TX mode and
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
     * \param packet packet received The packet length is the one set through
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
    
    /**
     * Destroyer
     */
    ~Cc2520Mapping(){};

    friend class Transceiver;
private:
    /**
     * Constructor
     */
    Cc2520Mapping();

    Cc2520Mapping(const Cc2520Mapping&);
    Cc2520Mapping& operator=(const Cc2520Mapping&);
    
    Cc2520& cc2520 ;//= Cc2520::instance();
    unsigned short length; ///< Packet length
};

#endif //CC2520_H
