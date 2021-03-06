/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "video-stream-client.h"

// seqts header
#include "seq-ts-header.h"

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("VideoStreamClientApplication");

    NS_OBJECT_ENSURE_REGISTERED(VideoStreamClient);

    TypeId
    VideoStreamClient::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::VideoStreamClient")
                                .SetParent<Application>()
                                .SetGroupName("Applications")
                                .AddConstructor<VideoStreamClient>()
                                .AddAttribute("RemoteAddress", "The destination address of the outbound packets",
                                              AddressValue(),
                                              MakeAddressAccessor(&VideoStreamClient::m_peerAddress),
                                              MakeAddressChecker())
                                .AddAttribute("RemotePort", "The destination port of the outbound packets",
                                              UintegerValue(5000),
                                              MakeUintegerAccessor(&VideoStreamClient::m_peerPort),
                                              MakeUintegerChecker<uint16_t>())
                                .AddAttribute("PacketNum", "The number of packets per frame",
                                              UintegerValue(100),
                                              MakeUintegerAccessor(&VideoStreamClient::m_packetNum),
                                              MakeUintegerChecker<uint32_t>());
        return tid;
    }

    VideoStreamClient::VideoStreamClient()
    {
        NS_LOG_FUNCTION(this);
        m_initialDelay = 3;
        m_currentBufferSize = 0;
        m_frameSize = 0;
        m_frameRate = 20;
        m_lastRecvFrame = 0;
        m_bufferEvent = EventId();
        m_sendEvent = EventId();
        m_expectedSeq = 0;
        m_retransPktSize = 100;
        m_frameFront = 0;
        m_frameBufferSize = 0;
    }

    VideoStreamClient::~VideoStreamClient()
    {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
    }

    void
    VideoStreamClient::SetRemote(Address ip, uint16_t port)
    {
        NS_LOG_FUNCTION(this << ip << port);
        m_peerAddress = ip;
        m_peerPort = port;
    }

    void
    VideoStreamClient::SetRemote(Address addr)
    {
        NS_LOG_FUNCTION(this << addr);
        m_peerAddress = addr;
    }

    void
    VideoStreamClient::DoDispose(void)
    {
        NS_LOG_FUNCTION(this);
        Application::DoDispose();
    }

    void
    VideoStreamClient::StartApplication(void)
    {
        NS_LOG_FUNCTION(this);

        if (m_socket == 0)
        {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
            m_socket = Socket::CreateSocket(GetNode(), tid);
            if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
            {
                if (m_socket->Bind() == -1)
                {
                    NS_FATAL_ERROR("Failed to bind socket");
                }
                m_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
            }
            else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
            {
                if (m_socket->Bind6() == -1)
                {
                    NS_FATAL_ERROR("Failed to bind socket");
                }
                m_socket->Connect(m_peerAddress);
            }
            else if (InetSocketAddress::IsMatchingType(m_peerAddress) == true)
            {
                if (m_socket->Bind() == -1)
                {
                    NS_FATAL_ERROR("Failed to bind socket");
                }
                m_socket->Connect(m_peerAddress);
            }
            else if (Inet6SocketAddress::IsMatchingType(m_peerAddress) == true)
            {
                if (m_socket->Bind6() == -1)
                {
                    NS_FATAL_ERROR("Failed to bind socket");
                }
                m_socket->Connect(m_peerAddress);
            }
            else
            {
                NS_ASSERT_MSG(false, "Incompatible address type: " << m_peerAddress);
            }
        }
        m_socket->SetRecvCallback(MakeCallback(&VideoStreamClient::HandleRead, this));
        m_sendEvent = Simulator::Schedule(MilliSeconds(1.0), &VideoStreamClient::Send, this);
        m_bufferEvent = Simulator::Schedule(Seconds(m_initialDelay), &VideoStreamClient::ReadFromBuffer, this);
    }

    void
    VideoStreamClient::StopApplication()
    {
        NS_LOG_FUNCTION(this);

        if (m_socket != 0)
        {
            m_socket->Close();
            m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
            m_socket = 0;
        }

        Simulator::Cancel(m_bufferEvent);
    }

    void
    VideoStreamClient::Send(void)
    {
        NS_LOG_FUNCTION(this);
        NS_ASSERT(m_sendEvent.IsExpired());

        // Server??? Connection??? ?????? ??????
        Ptr<Packet> firstPacket = Create<Packet>(10);
        m_socket->Send(firstPacket);

        if (Ipv4Address::IsMatchingType(m_peerAddress))
        {
            // NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client sent 10 bytes to " << Ipv4Address::ConvertFrom(m_peerAddress) << " port " << m_peerPort);
        }
        else if (Ipv6Address::IsMatchingType(m_peerAddress))
        {
            // NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client sent 10 bytes to " << Ipv6Address::ConvertFrom(m_peerAddress) << " port " << m_peerPort);
        }
        else if (InetSocketAddress::IsMatchingType(m_peerAddress))
        {
            // NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client sent 10 bytes to " << InetSocketAddress::ConvertFrom(m_peerAddress).GetIpv4() << " port " << InetSocketAddress::ConvertFrom(m_peerAddress).GetPort());
        }
        else if (Inet6SocketAddress::IsMatchingType(m_peerAddress))
        {
            // NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client sent 10 bytes to " << Inet6SocketAddress::ConvertFrom(m_peerAddress).GetIpv6() << " port " << Inet6SocketAddress::ConvertFrom(m_peerAddress).GetPort());
        }
    }

    void
    VideoStreamClient::SendRetransRequest(void)
    {
        if (!m_retransBuffer.empty())
        {
            // ?????? ??????
            Ptr<Packet> retransRequestPacket = Create<Packet>(m_retransPktSize);

            // seqTsHeader ??????
            SeqTsHeader seqTs;
            uint32_t retransSeq = m_retransBuffer.front();
            m_retransBuffer.pop();
            seqTs.SetSeq(retransSeq);
            // NS_LOG_INFO("[Client] At time " << Simulator::Now().GetSeconds() << " retrans request for pkt # " << retransSeq);
            // ????????? seq header ????????? + ??????
            retransRequestPacket->AddHeader(seqTs);
            m_socket->Send(retransRequestPacket);
            // retransBuffer??? ???????????? ?????? ?????? retrans ?????? ??????
            if (!m_retransBuffer.empty())
                m_retransEvent = Simulator::Schedule(MilliSeconds(1.0), &VideoStreamClient::SendRetransRequest, this);
        }
    }

    uint32_t
    VideoStreamClient::ReadFromBuffer(void)
    {
        uint32_t count = 0; // ???????????? ????????? ??????
        if (m_frameBufferSize < m_frameRate)
        {
            // ???????????? ????????? ????????? ????????????
            count = 0;
            for (uint32_t i = m_frameFront; i < m_frameFront + m_frameBufferSize; i++)
            {
                if (m_frameBuffer[i] > 0)
                {
                    count += 1;
                    m_frameBuffer[i] = 0; // i?????? ????????? ??????
                }
            }
            m_frameFront = m_frameFront + m_frameBufferSize;             // ??????????????? ???????????? ?????????????????? ??? ??????
            m_frameBufferSize -= m_frameBufferSize;                      // ????????? ?????? ????????? ????????? ????????? ????????? ?????????
            NS_LOG_INFO(Simulator::Now().GetSeconds() << "\t" << count); // ????????? ????????? ?????? ??????
            m_bufferEvent = Simulator::Schedule(Seconds(1.0), &VideoStreamClient::ReadFromBuffer, this);
            return (-1); // not consume frame
        }
        else // consume frame
        {
            // m_frameRate????????? ????????????
            count = 0;
            for (uint32_t i = m_frameFront; i < m_frameFront + m_frameRate; i++)
            {
                if (m_frameBuffer[i] > 0)
                {
                    count += 1;
                    m_frameBuffer[i] = 0; // i?????? ????????? ??????
                }
            }
            m_frameFront = m_frameFront + m_frameRate;                   // ??????????????? ???????????? ?????????????????? ??? ??????
            m_frameBufferSize -= m_frameRate;                            // ????????? ?????? ????????? ????????? ????????? ????????? ?????????
            NS_LOG_INFO(Simulator::Now().GetSeconds() << "\t" << count); // ????????? ????????? ?????? ??????
            m_bufferEvent = Simulator::Schedule(Seconds(1.0), &VideoStreamClient::ReadFromBuffer, this);
            return (m_frameBufferSize);
        }
    }

    void VideoStreamClient::HandleRead(Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION(this << socket);

        Ptr<Packet> packet;
        Address from;
        Address localAddress;
        while ((packet = socket->RecvFrom(from)))
        {
            socket->GetSockName(localAddress);
            if (InetSocketAddress::IsMatchingType(from))
            {
                uint32_t seqNum;   // ?????? ??? ????????? ???????????? ?????? ??????(seq)
                uint32_t frameNum; // ?????? ?????? ?????? ????????? ?????? ???????????? ?????? (seq ????????? ?????? ??????)
                uint32_t pktSize = packet->GetSize();
                SeqTsHeader seqTs;
                packet->RemoveHeader(seqTs);
                seqNum = seqTs.GetSeq();
                frameNum = seqNum / m_packetNum;

                // ??????????????? ????????? ????????? ?????? ??????
                if (m_expectedSeq <= seqNum)
                {
                    // seq??? ????????? ??????
                    if (m_expectedSeq == seqNum)
                    {
                        m_expectedSeq++;
                    }
                    // seq??? ????????? ??? ??????(?????? ????????? ??????) => ????????? ?????? ????????????
                    else
                    {
                        if (seqNum > m_expectedSeq)
                        {
                            // t = [m_expectedSeq ~ seqNum - 1]????????? ????????? ??????
                            // m_retransBuffer??? t?????? ?????????
                            for (uint32_t i = m_expectedSeq; i < seqNum; i++)
                                m_retransBuffer.push(i);
                            // m_retransEvent??? SendRetrans(void) ???????????? ?????????
                            m_retransEvent = Simulator::Schedule(MilliSeconds(1.0), &VideoStreamClient::SendRetransRequest, this);
                        }
                        // m_expectedSeq ?????? ??? m_frameSize ??????
                        m_expectedSeq = seqNum + 1;
                    }
                    // frame????????? ?????? packet ????????? ??????
                    if (frameNum == m_lastRecvFrame)
                    {
                        // ????????? ?????? ????????? ????????? ????????? ????????? ????????? ????????? ????????? ?????? => ?????? frame??? ????????? ?????? ?????????.
                        m_frameSize += pktSize;
                    }
                    else
                    {
                        // ????????? ????????? ??????
                        // NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << " new frame is saved : " << m_lastRecvFrame);
                        m_frameBuffer[m_lastRecvFrame] = m_frameSize; // ????????? ????????? ??????
                        m_frameBufferSize++;                          // frmaeBuffer??? ????????? ??????????????? ??????
                        m_lastRecvFrame = frameNum;                   // frame?????? ??????
                        m_frameSize = pktSize;                        // m_frameSize ??????
                    }
                }
                // ??????????????? ????????? ????????? ??????
                else
                {
                    // ??????????????? ?????? ????????? ????????? ?????? => 1) ???????????? ?????? ???????????? ?????? => ????????????????????? ?????? | 2) ?????? ????????? ???????????? ?????? => ??????
                    if (m_frameBuffer[frameNum] > 0)
                    {
                        m_frameBuffer[frameNum] += packet->GetSize();
                    }
                }
            }
        }
    }

} // namespace ns3