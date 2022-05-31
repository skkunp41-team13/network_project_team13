/*****************************************************
 *
 * File:  videoStreamTest.cc
 *
 * Explanation:  This script modifies the tutorial first.cc
 *               to test the video stream application.
 *
 *****************************************************/
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

//#define NS3_LOG_ENABLE

/**
 * @brief The test cases include:
 * 1. P2P network with 1 server and 1 client
 * 2. Wireless network with 1 server and 1 clients
 */

NS_LOG_COMPONENT_DEFINE("VideoStreamTest");

int main(int argc, char *argv[])
{
  CommandLine cmd;
  uint32_t _case = 1;
  uint32_t _pktPerFrame = 100;
  cmd.AddValue("case", "which case?", _case);
  cmd.AddValue("pktPerFrame", "# of packets per frame", _pktPerFrame);
  cmd.Parse(argc, argv);

  Time::SetResolution(Time::NS);
  LogComponentEnable("VideoStreamClientApplication", LOG_LEVEL_INFO);

  if (_case == 1)
  {
    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    VideoStreamClientHelper videoClient(interfaces.GetAddress(0), 5000);
    videoClient.SetAttribute("PacketNum", UintegerValue(_pktPerFrame));
    ApplicationContainer clientApp = videoClient.Install(nodes.Get(1));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(100.0));

    VideoStreamServerHelper videoServer(5000);
    videoServer.SetAttribute("PacketNum", UintegerValue(_pktPerFrame));

    ApplicationContainer serverApp = videoServer.Install(nodes.Get(0));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(100.0));

    pointToPoint.EnablePcap("videoStream", devices.Get(1), false);
    Simulator::Run();
    Simulator::Destroy();
  }
  else if (_case == 2)
  {
    // Create Nodes : Make 1 STA and 1 AP
    NodeContainer wifiStaNode;
    wifiStaNode.Create(1);
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    // Create PHY layer (wireless channel)
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.SetChannel(channel.Create());

    // Create MAC layer
    WifiMacHelper mac;
    Ssid ssid = Ssid("videoStreamTest");
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));

    // Create WLAN setting
    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211n_5GHZ);
    // wifi.SetRemoteStationManager("ns3::AarfWifiManager");
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager");

    // Create NetDevices
    NetDeviceContainer staDevice;
    staDevice = wifi.Install(phy, mac, wifiStaNode);

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevice;
    apDevice = wifi.Install(phy, mac, wifiApNode);

    // Create Network layer
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNode);

    Ipv4AddressHelper address;

    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign(apDevice);
    Ipv4InterfaceContainer wifiInterface;
    wifiInterface = address.Assign(staDevice);

    // Locate Nodes
    // Setting mobility model
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0)); // Add Vector x=0, y=0, z=0
    positionAlloc->Add(Vector(1.0, 0.0, 0.0)); // Add Vector x=1, y=0, z=0
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNode);

    // Create Transport layer (UDP)
    VideoStreamServerHelper videoServer(5000);
    videoServer.SetAttribute("PacketNum", UintegerValue(_pktPerFrame));

    ApplicationContainer serverApp = videoServer.Install(wifiApNode.Get(0));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(100.0));

    VideoStreamClientHelper videoClient(apInterface.GetAddress(0), 5000);
    videoClient.SetAttribute("PacketNum", UintegerValue(_pktPerFrame));
    ApplicationContainer clientApp = videoClient.Install(wifiStaNode.Get(0));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(100.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Simulator::Stop(Seconds(10.0));

    phy.EnablePcap("wifi-videoStream", apDevice.Get(0));
    AnimationInterface anim("wifi-1-1.xml");
    Simulator::Run();
    Simulator::Destroy();
  }
  return 0;
}