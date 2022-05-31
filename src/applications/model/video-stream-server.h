/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef VIDEO_STREAM_SERVER_H
#define VIDEO_STREAM_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/string.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"


//#include <fstream>
#include <unordered_map>
namespace ns3 {

    class Socket;
    class Packet;

    /**
     * @brief A Video Stream Server
     */
    class VideoStreamServer : public Application
    {
    public:
        /**
         * @brief Get the type ID.
         *
         * @return the object TypeId
         */
        static TypeId GetTypeId(void);

        VideoStreamServer();

        virtual ~VideoStreamServer();

    protected:
        virtual void DoDispose(void);

    private:

        virtual void StartApplication(void);
        virtual void StopApplication(void);

        /**
         * @brief The information required for each client.
         */
        typedef struct ClientInfo
        {
            Address m_address; //!< Address
            uint32_t m_sent; //!< Counter for sent frames
            EventId m_sendEvent; //! Send event used by the client
        } ClientInfo; //! To be compatible with C language

        /**
         * @brief Send a packet with specified size.
         *
         * @param packetSize the number of bytes for the packet to be sent
         */
        void SendPacket(ClientInfo* client, uint32_t packetSize);

        /**
         * @brief Send the video frame to the given ipv4 address.
         *
         * @param ipAddress ipv4 address
         */
        void Send(uint32_t ipAddress);

        /**
         * @brief Handle a packet reception.
         *
         * This function is called by lower layers.
         *
         * @param socket the socket the packet was received to
         */
        void HandleRead(Ptr<Socket> socket);

        uint32_t GetSeqNum(void);

        void AddAckSeqNum(uint32_t seqNum);

        Time m_interval; //!< Packet inter-send time
        uint32_t m_maxPacketSize; //!< Maximum size of the packet to be sent
        Ptr<Socket> m_socket; //!< Socket

        uint16_t m_port; //!< The port 
        Address m_local; //!< Local multicast address

        uint32_t m_packetNum;
        uint32_t m_nextSeqNum;
        uint32_t m_waitingSeqNum;
        uint32_t* m_sendQueue; // send buffer
        uint32_t m_sendQueueSize;
        uint32_t m_sendQueueFront;
        uint32_t m_sendQueueBack;

        std::unordered_map<uint32_t, ClientInfo*> m_clients; //!< Information saved for each client
    };

} // namespace ns3


#endif /* VIDEO_STREAM_SERVER_H */