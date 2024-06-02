/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef LS_MESSAGE_H
#define LS_MESSAGE_H

#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/object.h"
#include "ns3/packet.h"


using namespace ns3;

#define IPV4_ADDRESS_SIZE 4

class LSMessage : public Header
  {
  public:
    LSMessage();
    virtual ~LSMessage();

    //NEW: adding message types for the outgoing hello (req) and the hell0 ack (rsp)
    enum MessageType
      {
      PING_REQ,
      PING_RSP,
      HELLO_REQ,
      HELLO_RSP
      };

    LSMessage(LSMessage::MessageType messageType, uint32_t sequenceNumber, uint8_t ttl, Ipv4Address originatorAddress);

    /**
     *  \brief Sets message type
     *  \param messageType message type
     */
    void SetMessageType(MessageType messageType);

    /**
     *  \returns message type
     */
    MessageType GetMessageType() const;

    /**
     *  \brief Sets Sequence Number
     *  \param sequenceNumber Sequence Number of the request
     */
    void SetSequenceNumber(uint32_t sequenceNumber);

    /**
     *  \returns Sequence Number
     */
    uint32_t GetSequenceNumber() const;

    /**
     *  \brief Sets Originator IP Address
     *  \param originatorAddress Originator IPV4 address
     */
    void SetOriginatorAddress(Ipv4Address originatorAddress);

    /**
     *  \returns Originator IPV4 address
     */
    Ipv4Address GetOriginatorAddress() const;

    /**
     *  \brief Sets Time To Live of the message
     *  \param ttl TTL of the message
     */
    void SetTTL(uint8_t ttl);

    /**
     *  \returns TTL of the message
     */
    uint8_t GetTTL() const;

  private:
    /**
     *  \cond
     */
    MessageType m_messageType;
    uint32_t m_sequenceNumber;
    Ipv4Address m_originatorAddress;
    uint8_t m_ttl;
    /**
     *  \endcond
     */
  public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    void Print(std::ostream& os) const;
    uint32_t GetSerializedSize(void) const;
    void Serialize(Buffer::Iterator start) const;
    uint32_t Deserialize(Buffer::Iterator start);

  struct PingReq
      {
      void Print(std::ostream& os) const;
      uint32_t GetSerializedSize(void) const;
      void Serialize(Buffer::Iterator& start) const;
      uint32_t Deserialize(Buffer::Iterator& start);
      // Payload
      Ipv4Address destinationAddress;
      std::string pingMessage;
      };

    struct PingRsp
      {
      void Print(std::ostream& os) const;
      uint32_t GetSerializedSize(void) const;
      void Serialize(Buffer::Iterator& start) const;
      uint32_t Deserialize(Buffer::Iterator& start);
      // Payload
      Ipv4Address destinationAddress;
      std::string pingMessage;
      };

     //[DONE] TODO: add strucs for hello request
    struct HelloReq
    {
      //printing, getting serialize size, serialize, deserialize
      void Print(std::ostream& os) const;
      uint32_t GetSerializedSize(void) const;
      void Serialize(Buffer::Iterator& start) const;
      uint32_t Deserialize(Buffer::Iterator& start);

      //going to address
      Ipv4Address destinationAddress;

      //whats being sent - unsure if added payload needed
      std::string helloMessage;
    };

     //[DONE] TODO: add strucs for hello ack
    struct HelloRsp
    {
      //printing, getting serialize size, serialize, deserialize
      void Print(std::ostream& os) const;
      uint32_t GetSerializedSize(void) const;
      void Serialize(Buffer::Iterator& start) const;
      uint32_t Deserialize(Buffer::Iterator& start);
      Ipv4Address sourceAddress;
            //no additional payload

      //going to address
      Ipv4Address destinationAddress;

      //whats being sent - unsure if added payload needed
      std::string helloMessage;
    };

    //TODO: add a new neighborhood information dynamic arr (vector) where
    //where we can add each element <int><int>

    //TODO: struct with LSAdvertisement 

  private:
    struct
      {
      PingReq pingReq;
      PingRsp pingRsp;
     //[DONE] TODO: Add the hello and hello ack message types to the struct
      HelloReq helloReq;
      HelloRsp helloRsp;
      //TODO: add the the link state advert type
      } m_message;

  public:
    /**
     *  \returns PingReq Struct
     */
    PingReq GetPingReq();
    //[DONE]: TODO: Add the new getter for the hello request  
    HelloReq GetHelloReq();

    //TODO: Add the new getter for the link state advert


    /**
     *  \brief Sets PingReq message params
     *  \param message Payload String
     */

    void SetPingReq(Ipv4Address destinationAddress, std::string message);


    //[DONE] TODO: Add new setter for the Hello req 
    void SetHelloReq(Ipv4Address destinationAddress, std::string message);

    //TODO: Add new setter for the LS advert 


    /**
     * \returns PingRsp Struct
     */
    PingRsp GetPingRsp();
    /**
     *  \brief Sets PingRsp message params
     *  \param message Payload String
     */

     //DONE: TODO: Add new setter for the hello Response 
      HelloRsp GetHelloRsp();


    void SetPingRsp(Ipv4Address destinationAddress, std::string message);

    //DONE: TODO: Add new setter for the hello response
    void SetHelloRsp(Ipv4Address destinationAddress, std::string message);

  }; // class LSMessage


static inline std::ostream&
operator<< (std::ostream& os, const LSMessage& message)
  {
  message.Print(os);
  return os;
  }

#endif

//~ resolve buggy push
