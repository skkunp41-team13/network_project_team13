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
                                .AddAttribute("PacketsPerFrame", "The number of packets per frame",
                                              UintegerValue(2831),
                                              MakeUintegerAccessor(&VideoStreamClient::m_pktsPerFrame)
                                                  MakeUintegerChecker<uint32_t>());
        return tid;
    }

    VideoStreamClient::VideoStreamClient()
    {
        NS_LOG_FUNCTION(this);
        m_initialDelay = 3;
        m_lastBufferSize = 0;
        m_currentBufferSize = 0;
        m_frameSize = 0;
        m_frameRate = 25;
        m_videoLevel = 3;
        m_stopCounter = 0;
        m_lastRecvFrame = 1e6;
        m_rebufferCounter = 0;
        m_bufferEvent = EventId();
        m_sendEvent = EventId();
        m_recvSeq = 0;
        m_retransPktSize = 100;
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

        // Server와 Connection을 위해 사용
        uint8_t dataBuffer[10];
        sprintf((char *)dataBuffer, "%hu", 0);
        Ptr<Packet> firstPacket = Create<Packet>(dataBuffer, 10);
        m_socket->Send(firstPacket);

        if (Ipv4Address::IsMatchingType(m_peerAddress))
        {
            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client sent 10 bytes to " << Ipv4Address::ConvertFrom(m_peerAddress) << " port " << m_peerPort);
        }
        else if (Ipv6Address::IsMatchingType(m_peerAddress))
        {
            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client sent 10 bytes to " << Ipv6Address::ConvertFrom(m_peerAddress) << " port " << m_peerPort);
        }
        else if (InetSocketAddress::IsMatchingType(m_peerAddress))
        {
            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client sent 10 bytes to " << InetSocketAddress::ConvertFrom(m_peerAddress).GetIpv4() << " port " << InetSocketAddress::ConvertFrom(m_peerAddress).GetPort());
        }
        else if (Inet6SocketAddress::IsMatchingType(m_peerAddress))
        {
            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client sent 10 bytes to " << Inet6SocketAddress::ConvertFrom(m_peerAddress).GetIpv6() << " port " << Inet6SocketAddress::ConvertFrom(m_peerAddress).GetPort());
        }
    }

    void
    VideoStreamClient::SendRetransRequest(void)
    {
        if (!m_retransBuffer.empty())
        {
            // 패킷 생성
            Ptr<Packet> retransRequestPacket = Create<Packet>(m_retransPktSize);

            // seqTsHeader 생성
            SeqTsHeader seqTs;
            uint32_t retransSeq = m_retransBuffer.front();
            m_retransBuffer.pop();
            seqTs.SetSeq(retransSeq);

            // 패킷에 seq header 붙이기
            retransRequestPacket->AddHeader(seqTs);
            m_socket->Send(retransRequestPacket);

            if (!m_retransBuffer.empty())
            {
                m_retransEvent = Simulator::Schedule(MilliSeconds(1.0), &VideoStreamClient::SendRetransRequest, this);
            }
        }
    }

    uint32_t
    VideoStreamClient::ReadFromBuffer(void)
    {
        // NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " s, last buffer size: " << m_lastBufferSize << ", current buffer size: " << m_currentBufferSize);
        // 재생되고 있는 frame 보다 현재 buffer에 보유중인 프레임 수가 더 작은 경우
        if (m_currentBufferSize < m_frameRate)
        {
            // m_lastBufferSize = 이전 시퀀스 에서의 bufferSize
            // m_currentBufferSize = 현재 시퀀스 에서의 bufferSize => 이 둘이 같다는 이야기는 시퀀스가 지났음에도 buffer에 누직된 프레임이 소비 되지 않았음을 의미
            if (m_lastBufferSize == m_currentBufferSize)
            {
                m_stopCounter++;
                // If the counter reaches 3, which means the client has been waiting for 3 sec, and no packets arrived.
                // In this case, we think the video streaming has finished, and there is no need to schedule the event.
                if (m_stopCounter < 3)
                {
                    m_bufferEvent = Simulator::Schedule(Seconds(1.0), &VideoStreamClient::ReadFromBuffer, this);
                }
            }
            else
            {
                NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << " s: Not enough frames in the buffer, rebuffering!");
                m_stopCounter = 0; // reset the stopCounter
                m_rebufferCounter++;
                m_bufferEvent = Simulator::Schedule(Seconds(1.0), &VideoStreamClient::ReadFromBuffer, this);
            }

            m_lastBufferSize = m_currentBufferSize;
            return (-1);
        }
        else
        {
            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << " s: Play video frames from the buffer");
            if (m_stopCounter > 0)
                m_stopCounter = 0; // reset the stopCounter
            if (m_rebufferCounter > 0)
                m_rebufferCounter = 0; // reset the rebufferCounter
            m_currentBufferSize -= m_frameRate;

            m_bufferEvent = Simulator::Schedule(Seconds(1.0), &VideoStreamClient::ReadFromBuffer, this);
            m_lastBufferSize = m_currentBufferSize;
            return (m_currentBufferSize);
        }
    }

    void
    VideoStreamClient::HandleRead(Ptr<Socket> socket)
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
                uint8_t recvData[packet->GetSize()];
                packet->CopyData(recvData, packet->GetSize());
                uint32_t seqNum;   // 패킷 내 몇번째 패킷인지 담는 변수(seq)
                uint32_t frameNum; // 현재 받고 있는 패킷이 속한 프레임의 번호 (seq 번호로 부터 추출)
                SeqTsHeader seqTs;

                // sscanf((char *)recvData, "%u", &frameNum);
                packet->RemoveHeader(seqTs);
                seqNum = seqTs.GetSeq();
                frameNum = seqNum / m_pktsPerFrame;

                // seq가 연속인 경우
                if (m_recvSeq == seqNum)
                {
                    m_recvSeq++;
                    if (frameNum == m_lastRecvFrame)
                    {
                        // 이전에 받은 패킷의 프레임 번호와 동일한 프레임 번호의 패킷인 경우 => 받은 frame의 크기를 증가 시킨다.
                        m_frameSize += packet->GetSize();
                    }
                    else
                    {
                        // 새로운 프레임 번호
                        NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client received frame " << frameNum + 1 << " and " << m_frameSize << " bytes from " << InetSocketAddress::ConvertFrom(from).GetIpv4() << " port " << InetSocketAddress::ConvertFrom(from).GetPort());
                        m_currentBufferSize++;      // frame 한개 추가
                        m_lastRecvFrame = frameNum; // frame번호 갱신
                        m_frameSize = packet->GetSize();
                    }
                }
                else
                {
                    // 일부 와야될 패킷이 버려진 경우 => 버려진 패킷들에 대해서 재전송 요청
                    if (seqNum > m_recvSeq)
                    {
                        // t = [m_recvSeq ~ seqNum - 1]번까지 재전송 요청
                        // m_retransBuffer에 t들을 넣는다
                        for (uint i = m_recvSeq; i < seqNum; i++)
                        {
                            m_retransBuffer.push(i);
                        }
                        // m_retransEvent에 SendRetrans(void) 이벤트를 트리거
                        m_retransEvent = Simulator::Schedule(MilliSeconds(1.0), &VideoStreamClient::SendRetransRequest, this);
                        // m_recvSeq 변경 후 m_frameSize 변경
                        m_recvSeq = seqNum + 1;
                        if (frameNum == m_lastRecvFrame)
                        {
                            // 이전에 받은 패킷의 프레임 번호와 동일한 프레임 번호의 패킷인 경우 => 받은 frame의 크기를 증가 시킨다.
                            m_frameSize += packet->GetSize();
                        }
                        else
                        {
                            // 새로운 프레임 번호
                            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s client received frame " << frameNum + 1 << " and " << m_frameSize << " bytes from " << InetSocketAddress::ConvertFrom(from).GetIpv4() << " port " << InetSocketAddress::ConvertFrom(from).GetPort());
                            m_currentBufferSize++;      // frame 한개 추가
                            m_lastRecvFrame = frameNum; // frame번호 갱신
                            m_frameSize = packet->GetSize();
                        }
                    }
                    // seqNum < m_recvSeq => 재전송 요청했던 패킷이 도착한 경우 => Consume되지 않은 무시
                    //
                }
            }
        }
    }

} // namespace ns3