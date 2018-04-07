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
 *
 *
 * Yanbing Jiang 100917795
 * Karl Tanguay-Verreault 100954279
 * Eric Cho 100763765
 * Nada El Tayeby Ahmed 100925827
 */
#include <stdio.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1   n2   n3   n4
//    point-to-point  |    |    |    |
//                    ================
//                      LAN 10.1.2.0


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SecondScriptExample");

int MacRxCount = 0; //Mac received
void MacRxFunc (Ptr<const Packet> p)
{
   MacRxCount++;
}

int MacTxCount = 0;
void MacTxFunc (Ptr<const Packet> p)
{
   MacTxCount++;
}

int MacTxDropCount = 0;
void MacTxDropFunc (Ptr<const Packet> p)
{
   MacTxDropCount++;
}

int PhyRxEndCount = 0;
void PhyRxEndFunc (Ptr<const Packet> p)
{
   PhyRxEndCount++;
}

int PhyTxEndCount = 0;
void PhyTxEndFunc (Ptr<const Packet> p)
{
   PhyTxEndCount++;
}

int PhyTxDropCount = 0;
void PhyTxDropFunc (Ptr<const Packet> p)
{
   PhyTxDropCount++;
}


int PhyRxDropCount = 0;
void PhyRxDropFunc (Ptr<const Packet> p)
{
   PhyRxDropCount++;
}

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 4;

  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  nCsma = nCsma == 0 ? 1 : nCsma;

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (0));
  stack.Install (csmaNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (11.0));

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1000));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0));
  clientApps.Start (Seconds (1.0));
  clientApps.Stop (Seconds (11.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  pointToPoint.EnablePcapAll ("second");
  csma.EnablePcap ("second", csmaDevices.Get (1), true);

  Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::CsmaNetDevice/MacRx",MakeCallback(&MacRxFunc)); //MacRx

  Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::CsmaNetDevice/MacTx",MakeCallback(&MacTxFunc)); //MacTx

  Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::CsmaNetDevice/MacTxDrop",MakeCallback(&MacTxDropFunc)); //MacTxDrop: packet have been dropped by the device before transmission

  Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::CsmaNetDevice/PhyTxEnd",MakeCallback(&PhyTxEndFunc)); //PhyTxEnd : packet have been sucessfully transmitted over the chanel

  Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::CsmaNetDevice/PhyRxEnd",MakeCallback(&PhyRxEndFunc)); //PhyRxEnd : packet have been sucessfully received from the channel to the device

  Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::CsmaNetDevice/PhyTxDrop",MakeCallback(&PhyTxDropFunc)); //PhyTxDrop packet dropped during transmission

  Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::CsmaNetDevice/PhyRxDrop",MakeCallback(&PhyRxDropFunc)); //PhyRxDrop: packet dropped during reception


  Simulator::Run ();
  Simulator::Destroy ();
  printf ("\n Tracing Starts. \n\n");

  std::cout << "MacRx Counter value " <<MacRxCount << std::endl;

  std::cout << "MacTx Counter value " <<MacTxCount << std::endl;

  std::cout << "MacTxDrop Counter value " <<MacTxDropCount << std::endl;

  std::cout << "PhyTxEnd Counter value " <<PhyTxEndCount << std::endl;

  std::cout << "PhyRxEnd Counter value " <<PhyRxEndCount << std::endl;

  std::cout << "PhyTxDrop Counter value " <<PhyTxDropCount << std::endl;

  std::cout << "PhyRxDrop Counter value " <<PhyRxDropCount << std::endl;
  return 0;
}
