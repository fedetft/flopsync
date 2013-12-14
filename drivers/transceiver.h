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
#ifndef TRANSCEIVER_H
#define	TRANSCEIVER_H

/**
 * This super class allow to interface with a transceiver.
 */
class Transceiver 
{
public:
    
    /**
     * \return a singletone instance of transceiver used to define the second:
     * #define USE_TRANSCEIVER_CC2520
     * #define USE_TRANSCEIVER_NRF25L01 
     */
    static Transceiver& instance();

    /**
     * Operating mode of the transceiver
     */
    enum Mode {
        TX,
        RX,
        SLEEP
    };


    /** 
     * \param mode the new nRF operating mode
     */
    virtual void setMode(Mode mode) = 0;

    /**
     * \param freq the new Transceiver operating frequency, in MHz, from 2400 to 2500
     */
    virtual void setFrequency(unsigned short freq) = 0;

    /**
     * Set the packet length used both for transmitting and receiving.
     * The transceiver sends packet of that length and ingores received packets
     * of different length.
     * \param length packet length, must be between 1 and 32, default is 16.
     */
    virtual void setPacketLength(unsigned short length) = 0;

    /**
     * \return the packet length 
     */
    virtual unsigned short getPacketLength() const = 0;

    /**
     * Set the device's addresss. It is used both in transmission and reception
     * \param address device address
     */
    virtual void setAddress(const unsigned char address[3]) = 0;

    /**
     * \return true if the nrf IRQ signal of the NRF24L01+ is asserted 
     */
    virtual bool irq() const = 0;

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
    virtual void writePacket(const void *packet) = 0;

    /**
     * \return true if a transmission slot is available in the TX fifo 
     */
    virtual bool isTxSlotAvailable() const = 0;

    /**
     * \return true if all packets in the TX fifo have been sent 
     */
    virtual bool allPacketsSent() const = 0;

    /**
     * Clears the IRQ signal that is set after a packet is transmitted
     */
    virtual void clearTxIrq() = 0;

    /**
     * Resets the value returned by irq() after a packet has been sent 
     */
    virtual void endWritePacket() = 0;

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
     * \param packet packet received. The packet length is the one set through
     * setPacketLength()
     */
    virtual void readPacket(void *packet) = 0;

    /**
     * After setting the mode to RX the radio is kept ready to receive but in
     * low power state. By calling this function the radio can actually start
     * receiving.
     */
    virtual void startReceiving() = 0;

    /**
     * \return true if a packet is available to be read from the RX fifo 
     */
    virtual bool isRxPacketAvailable() const = 0;

protected:
    virtual ~Transceiver(){}
    

};

#endif	//TRANSCEIVER_H

