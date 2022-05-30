/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef VIDEO_STREAM_HELPER_H
#define VIDEO_STREAM_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"

namespace ns3 {

/**
 * @brief Create a server application that sends the video frames to a client.
 */
class VideoStreamServerHelper
{
private:
  /**
   * Install an ns3::VideoStreamServer on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an VideoStreamServer will be installed.
   * \returns Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_factory; //!< Object factory.

public:
  /**
   * Create VideoStreamServerHelper which will make life easier for people trying
   * to set up VideoStreamServer.
   *
   * \param port The port the server will wait on for incoming packets
   */
  VideoStreamServerHelper (uint16_t port);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create a VideoStreamServerApplication on the specified Node.
   *
   * \param node The node on which to create the Application.  The node is
   *             specified by a Ptr<Node>.
   *
   * \returns An ApplicationContainer holding the Application created,
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Create a VideoStreamServerApplication on specified node
   *
   * \param nodeName The node on which to create the application.  The node
   *                 is specified by a node name previously registered with
   *                 the Object Name Service.
   *
   * \returns An ApplicationContainer holding the Application created.
   */
  ApplicationContainer Install (std::string nodeName) const;

  /**
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   *
   * Create one VideoStreamServer application on each of the Nodes in the
   * NodeContainer.
   *
   * \returns The applications created, one Application per Node in the 
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c) const;

};

class VideoStreamClientHelper
{
private:
  /**
   * Install an ns3::VideoStreamClient on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an VideoStreamClient will be installed.
   * \returns Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory; //!< Object factory.

public:
  /**
   * Create VideoStreamClientHelper which will make life easier for people trying
   * to set up VideoStreamClient. Use this variant with addresses that do
   * not include a port value (e.g., Ipv4Address and Ipv6Address).
   *
   * \param ip The IP address of the remote udp echo server
   * \param port The port number of the remote udp echo server
   */
  VideoStreamClientHelper (Address ip, uint16_t port);
  /**
   * Create VideoStreamClientHelper which will make life easier for people trying
   * to set up VideoStreamClient. Use this variant with addresses that do
   * include a port value (e.g., InetSocketAddress and Inet6SocketAddress).
   *
   * \param addr The address of the remote udp echo server
   */
  VideoStreamClientHelper (Address addr);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create a VideoStreamClientApplication on the specified node.  The Node
   * is provided as a Ptr<Node>.
   *
   * \param node The Ptr<Node> on which to create the VideoStreamClientApplication.
   *
   * \returns An ApplicationContainer that holds a Ptr<Application> to the 
   *          application created
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Create a VideoStreamClientApplication on the specified node.  The Node
   * is provided as a string name of a Node that has been previously 
   * associated using the Object Name Service.
   *
   * \param nodeName The name of the node on which to create the VideoStreamClientApplication
   *
   * \returns An ApplicationContainer that holds a Ptr<Application> to the 
   *          application created
   */
  ApplicationContainer Install (std::string nodeName) const;

  /**
   * \param c the nodes
   *
   * Create one VideoStreamClientApplication on each of the input nodes
   *
   * \returns the applications created, one application per input node.
   */
  ApplicationContainer Install (NodeContainer c) const;

};

} // namespace ns3

#endif /* VIDEO_STREAM_HELPER_H */