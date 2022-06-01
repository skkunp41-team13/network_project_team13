/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef VIDEO_STREAM_CLIENT_H
#define VIDEO_STREAM_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"

#include <queue>

#define MAX_VIDEO_LEVEL 6

namespace ns3
{

  class Socket;
  class Packet;

  /**
   * @brief A Video Stream Client
   */
  class VideoStreamClient : public Application
  {
  public:
    /**
     * @brief Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId(void);
    VideoStreamClient();
    virtual ~VideoStreamClient();

    /**
     * @brief Set the server address and port.
     *
     * @param ip server IP address
     * @param port server port
     */
    void SetRemote(Address ip, uint16_t port);
    /**
     * @brief Set the server address.
     *
     * @param addr server address
     */
    void SetRemote(Address addr);

  protected:
    virtual void DoDispose(void);

  private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    /**
     * @brief Send the packet to the remote server.
     */
    void Send(void);

    /**
     * @brief Send the packet to the remote server to require retransmission request.
     */
    void SendRetransRequest(void);

    /**
     * @brief Read data from the frame buffer. If the buffer does not have
     * enough frames, it will reschedule the reading event next second.
     *
     * @return the updated buffer size (-1 if the buffer size is smaller than the fps)
     */
    uint32_t ReadFromBuffer(void);

    /**
     * @brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * @param socket the socket the packet was received to
     */
    void HandleRead(Ptr<Socket> socket);

    Ptr<Socket> m_socket;  //!< Socket
    Address m_peerAddress; //!< Remote peer address
    uint16_t m_peerPort;   //!< Remote peer port

    uint16_t m_initialDelay;      //!< Seconds to wait before displaying the content
    uint32_t m_frameRate;         //!< Number of frames per second to be played
    uint32_t m_frameSize;         //!< Total size of packets from one frame
    uint32_t m_lastRecvFrame;     //!< Last received frame number
    uint32_t m_currentBufferSize; //!< Size of the frame buffer

    uint32_t m_packetNum;                 // frame당 패킷 개수
    uint32_t m_expectedSeq;               // 받아야 되는 packet seq 번호 (0부터 시작)
    uint32_t m_retransPktSize;            // 재전송 요청 패킷 사이즈
    std::queue<uint32_t> m_retransBuffer; // 재전송 요청할 seq 번호를 담는 큐
    uint32_t m_frameBuffer[32786];
    uint32_t m_frameFront;
    uint32_t m_frameBufferSize;

    EventId m_bufferEvent;  //!< Event to read from the buffer
    EventId m_sendEvent;    //!< Event to send data to the server
    EventId m_retransEvent; //!< 재전송 요청 이벤트
  };

} // namespace ns3

#endif /* VIDEO_STREAM_CLIENT_H */