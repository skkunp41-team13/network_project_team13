/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
//#include "ns3/trace-source-accessor.h"
#include "seq-ts-header.h"
#include "ns3/video-stream-server.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("VideoStreamServerApplication");

    NS_OBJECT_ENSURE_REGISTERED(VideoStreamServer);

    TypeId
        VideoStreamServer::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::VideoStreamServer")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<VideoStreamServer>()
            .AddAttribute("Interval", "The time to wait between packets",
                TimeValue(Seconds(0.01)),
                MakeTimeAccessor(&VideoStreamServer::m_interval),
                MakeTimeChecker())
            .AddAttribute("Port", "Port on which we listen for incoming packets.",
                UintegerValue(5000),
                MakeUintegerAccessor(&VideoStreamServer::m_port),
                MakeUintegerChecker<uint16_t>())
            .AddAttribute("MaxPacketSize", "The maximum size of a packet",
                UintegerValue(1400),
                MakeUintegerAccessor(&VideoStreamServer::m_maxPacketSize),
                MakeUintegerChecker<uint16_t>())
            ;
        return tid;
    }

    VideoStreamServer::VideoStreamServer()
    {
        NS_LOG_FUNCTION(this);
        m_socket = 0;

        m_nextSeqNum = 0;
        m_waitingSeqNum = 0;
        m_sendQueueSize = 32786;
        m_sendQueue = new uint32_t[m_sendQueueSize]();
        m_sendQueueFront = 0;
        m_sendQueueBack = 0;
    }

    VideoStreamServer::~VideoStreamServer()
    {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
    }

    void
        VideoStreamServer::DoDispose(void)
    {
        NS_LOG_FUNCTION(this);
        Application::DoDispose();
    }

    void
        VideoStreamServer::StartApplication(void)
    {
        NS_LOG_FUNCTION(this);

        if (m_socket == 0)
        {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
            m_socket = Socket::CreateSocket(GetNode(), tid);
            InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
            if (m_socket->Bind(local) == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            if (addressUtils::IsMulticast(m_local))
            {
                Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket>(m_socket);
                if (udpSocket)
                {
                    udpSocket->MulticastJoinGroup(0, m_local);
                }
                else
                {
                    NS_FATAL_ERROR("Error: Failed to join multicast group");
                }
            }
        }

        m_socket->SetAllowBroadcast(true);
        m_socket->SetRecvCallback(MakeCallback(&VideoStreamServer::HandleRead, this));
    }

    void
        VideoStreamServer::StopApplication()
    {
        NS_LOG_FUNCTION(this);


        if (m_socket != 0)
        {
            m_socket->Close();
            m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
            m_socket = 0;
        }

        for (auto iter = m_clients.begin(); iter != m_clients.end(); iter++)
        {
            Simulator::Cancel(iter->second->m_sendEvent);
        }

    }

    // Send Frame
    void
        VideoStreamServer::Send(uint32_t ipAddress)
    {
        NS_LOG_FUNCTION(this);

        uint32_t frameSize = 1400 * 2830 + 1000;
        uint32_t totalFrames = 60 * 25;
        ClientInfo* clientInfo = m_clients.at(ipAddress);

        NS_ASSERT(clientInfo->m_sendEvent.IsExpired());
        
        // 프레임을 패킷크기로 잘라서 전송, 
        /*
		for (uint i = 0; i < frameSize / m_maxPacketSize; i++)
        {
			SendPacket(clientInfo, m_maxPacketSize);
        }
		*/
        // 프레임에 남은 크기 (1000byte) 전송
        uint32_t remainder = frameSize % m_maxPacketSize;
        //SendPacket(clientInfo, remainder);
		
		while (m_nextSeqNum < (clientInfo->m_sent + 1) * 50)
        {
            SendPacket(clientInfo, m_maxPacketSize);

            if (m_nextSeqNum == (clientInfo->m_sent + 1) * 50 - 1)
            {
                SendPacket(clientInfo, remainder);
            }
        }

        NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s server sent frame " << clientInfo->m_sent << " and " << frameSize << " bytes to " << InetSocketAddress::ConvertFrom(clientInfo->m_address).GetIpv4() << " port " << InetSocketAddress::ConvertFrom(clientInfo->m_address).GetPort());

        clientInfo->m_sent += 1; // 보낸 프레임 개수 증가
        if (clientInfo->m_sent < totalFrames)
        {
            clientInfo->m_sendEvent = Simulator::Schedule(m_interval, &VideoStreamServer::Send, this, ipAddress);
        }
    }

    void
        VideoStreamServer::SendPacket(ClientInfo* client, uint32_t packetSize)
    {
        uint8_t dataBuffer[packetSize];
        sprintf((char*)dataBuffer, "%u", client->m_sent);
        Ptr<Packet> p = Create<Packet>(dataBuffer, packetSize);
        uint32_t seqNum = GetSeqNum();
        SeqTsHeader seqTs;
        seqTs.SetSeq(seqNum);
        p->AddHeader(seqTs);
        //m_txTrace(p);
        if (m_socket->SendTo(p, 0, client->m_address) < 0)
        {
            NS_LOG_INFO("Error while sending " << packetSize << "bytes to " << InetSocketAddress::ConvertFrom(client->m_address).GetIpv4() << " port " << InetSocketAddress::ConvertFrom(client->m_address).GetPort());
        }
    }

    void
        VideoStreamServer::HandleRead(Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION(this << socket);

        Ptr<Packet> packet;
        Address from;
        Address localAddress;
        SeqTsHeader seqTs;
        uint32_t seqNum;
        while ((packet = socket->RecvFrom(from)))
        {
			socket->GetSockName(localAddress);
            if (packet->GetSize() > 10)
			{
			packet->RemoveHeader(seqTs);
            seqNum = seqTs.GetSeq();
            }
			if (InetSocketAddress::IsMatchingType(from))
            {
                NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s server received " << packet->GetSize() << " bytes from " << InetSocketAddress::ConvertFrom(from).GetIpv4() << " port " << InetSocketAddress::ConvertFrom(from).GetPort() << " seq " << seqNum);

			uint32_t ipAddr = InetSocketAddress::ConvertFrom (from).GetIpv4 ().Get ();

      // the first time we received the message from the client
      if (m_clients.find (ipAddr) == m_clients.end ())
      {
        ClientInfo *newClient = new ClientInfo();
        newClient->m_sent = 0;
       // newClient->m_videoLevel = 3;
        newClient->m_address = from;
        // newClient->m_sendEvent = EventId ();
        m_clients[ipAddr] = newClient;
        newClient->m_sendEvent = Simulator::Schedule (Seconds (0.0), &VideoStreamServer::Send, this, ipAddr);
      }


            }
            if (packet->GetSize() > 10)
	        {
			AddAckSeqNum(seqNum);
            socket->GetSockName(localAddress);
            }
			//m_rxTrace(packet);
            //m_rxTraceWithAddresses(packet, from, localAddress);
        }
    }

    //get next sequence number
    uint32_t
        VideoStreamServer::GetSeqNum(void)
    {
        uint32_t seqNum;
        if (m_sendQueueFront != m_sendQueueBack)
        {
            seqNum = m_sendQueue[m_sendQueueFront++];
            if (m_sendQueueFront == m_sendQueueSize)
            {
                m_sendQueueFront = 0;
            }
            NS_LOG_INFO(seqNum << " Retransmission");
        }
        else
        {
            seqNum = m_nextSeqNum++;
        }

        return seqNum;
    }

    void
        VideoStreamServer::AddAckSeqNum(uint32_t seqNum)
    {
        if (seqNum < m_waitingSeqNum)
        {
            NS_LOG_INFO("Receive Retrans Packet: " << seqNum);
        }
        else if (seqNum == m_waitingSeqNum)
        {
            ++m_waitingSeqNum;
        }
        else
        {
            NS_LOG_INFO("Packet Loss: " << m_waitingSeqNum);
            for (uint32_t i = m_waitingSeqNum; i < seqNum; ++i)
            {
                if ((m_sendQueueBack + 1) % m_sendQueueSize == m_sendQueueFront)
                {
                    NS_LOG_INFO("Queue over flow");
                    break;
                }
                else
                {
                    m_sendQueue[m_sendQueueBack++] = i;
                    if (m_sendQueueBack == m_sendQueueSize)
                    {
                        m_sendQueueBack = 0;
                    }
                }
            }
            m_waitingSeqNum = seqNum + 1;
        }
    }

} // namespace ns3
