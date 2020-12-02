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

// Default network topology includes some number of AP nodes specified by
// the variable nWifis (defaults to two).  Off of each AP node, there are some
// number of STA nodes specified by the variable nStas (defaults to two).
// Each AP talks to its associated STA nodes.  There are bridge net devices
// on each AP node that bridge the whole thing into one network.
//
//      +-----+      +-----+            +-----+      +-----+
//      | STA |      | STA |            | STA |      | STA |
//      +-----+      +-----+            +-----+      +-----+
//    192.168.0.2  192.168.0.3        192.168.0.5  192.168.0.6
//      --------     --------           --------     --------
//      WIFI STA     WIFI STA           WIFI STA     WIFI STA
//      --------     --------           --------     --------
//        ((*)) (2)      ((*)) (3)      |      ((*)) (4)       ((*))(5)
//                                |
//              ((*))             |             ((*))
//             -------                         -------
//             WIFI AP   CSMA ========= CSMA   WIFI AP
//             -------   ----           ----   -------
//             ##############           ##############
//                 BRIDGE                   BRIDGE
//             ##############           ##############
//               192.168.0.1(0)           192.168.0.4(1)
//               +---------+              +---------+
//               | AP Node |              | AP Node |
//               +---------+              +---------+

#include "ns3/command-line.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/csma-helper.h"
#include "ns3/animation-interface.h"
#include "ns3/bridge-helper.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"

 
using namespace ns3;

int main (int argc, char *argv[])
{
  uint32_t nWifis = 2;
  uint32_t nStas = 2;
  bool sendIp = true;
  bool writeMobility = false;

  CommandLine cmd;
  cmd.AddValue ("nWifis", "Number of wifi networks", nWifis);
  cmd.AddValue ("nStas", "Number of stations per wifi network", nStas);
  cmd.AddValue ("SendIp", "Send Ipv4 or raw packets", sendIp);
  cmd.AddValue ("writeMobility", "Write mobility trace", writeMobility);
  cmd.Parse (argc, argv);

  NodeContainer backboneNodes;
  NetDeviceContainer backboneDevices;
  Ipv4InterfaceContainer backboneInterfaces;
  std::vector<NodeContainer> staNodes;
  std::vector<NetDeviceContainer> staDevices;
  std::vector<NetDeviceContainer> apDevices;
  std::vector<Ipv4InterfaceContainer> staInterfaces;
  std::vector<Ipv4InterfaceContainer> apInterfaces;

  InternetStackHelper stack;
  CsmaHelper csma;
  Ipv4AddressHelper ip;
  ip.SetBase ("192.168.0.0", "255.255.255.0");

  backboneNodes.Create (nWifis);
  stack.Install (backboneNodes);

  backboneDevices = csma.Install (backboneNodes);

  double wifiX = 0.0;

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  for (uint32_t i = 0; i < nWifis; ++i)
    {
      // calculate ssid for wifi subnetwork
      std::ostringstream oss;
      oss << "wifi-default-" << i;
      Ssid ssid = Ssid (oss.str ());

      NodeContainer sta;
      NetDeviceContainer staDev;
      NetDeviceContainer apDev;
      Ipv4InterfaceContainer staInterface;
      Ipv4InterfaceContainer apInterface;
      MobilityHelper mobility;
      BridgeHelper bridge;
      WifiHelper wifi;
      WifiMacHelper wifiMac;
      YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
      wifiPhy.SetChannel (wifiChannel.Create ());

      sta.Create (nStas);
      mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (wifiX),
                                     "MinY", DoubleValue (0.0),
                                     "DeltaX", DoubleValue (5.0),
                                     "DeltaY", DoubleValue (5.0),
                                     "GridWidth", UintegerValue (1),
                                     "LayoutType", StringValue ("RowFirst"));

      // setup the AP.
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (backboneNodes.Get (i));
      wifiMac.SetType ("ns3::ApWifiMac",
                       "Ssid", SsidValue (ssid));
      apDev = wifi.Install (wifiPhy, wifiMac, backboneNodes.Get (i));

      NetDeviceContainer bridgeDev;
      bridgeDev = bridge.Install (backboneNodes.Get (i), NetDeviceContainer (apDev, backboneDevices.Get (i)));

      // assign AP IP address to bridge, not wifi
      apInterface = ip.Assign (bridgeDev);

      // setup the STAs
      stack.Install (sta);
      mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                 "Mode", StringValue ("Time"),
                                 "Time", StringValue ("2s"),
                                 "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                                 "Bounds", RectangleValue (Rectangle (wifiX, wifiX + 5.0,0.0, (nStas + 1) * 5.0)));
      mobility.Install (sta);
      wifiMac.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid));
      staDev = wifi.Install (wifiPhy, wifiMac, sta);
      staInterface = ip.Assign (staDev);

      // save everything in containers.
      staNodes.push_back (sta);
      apDevices.push_back (apDev);
      apInterfaces.push_back (apInterface);
      staDevices.push_back (staDev);
      staInterfaces.push_back (staInterface);

      wifiX += 500.0;
    }

//   Address dest;
//   std::string protocol;
  
//       dest = InetSocketAddress (apInterfaces[0].GetAddress (0), 1025);
//       std::cout<<staInterfaces[1].GetAddress (1)<<"staInterfaces[1].GetAddress (1)\n";

//          std::cout<<staInterfaces[1].GetAddress (0)<<"staInterfaces[1].GetAddress (0)\n";
//        std::cout<<staInterfaces[0].GetAddress (0)<<"staInterfaces[0].GetAddress (0)\n";
//           std::cout<<staInterfaces[0].GetAddress (1)<<"staInterfaces[0].GetAddress (0)\n";
//              std::cout<<apInterfaces[0].GetAddress (0)<<"staInterfaces[0].GetAddress (0)\n";
//                      std::cout<<apInterfaces[1].GetAddress (0)<<"staInterfaces[0].GetAddress (0)\n";
     
//       protocol = "ns3::TcpSocketFactory";

//   OnOffHelper onoff = OnOffHelper (protocol, dest);
//   onoff.SetConstantRate (DataRate ("500000kb/s"));
//   ApplicationContainer apps = onoff.Install (staNodes[0].Get (0));
//   apps.Start (Seconds (0.5));
//   apps.Stop (Seconds (3.0));


 ApplicationContainer serverApp;
   uint32_t payloadSize;
      payloadSize = 1448; //bytes

  ///////////////
                      //TCP flow
                      uint16_t port = 50000;
                      Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
                      PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", localAddress);
                      serverApp = packetSinkHelper.Install (staNodes[0].Get (0));
                       serverApp.Start (Seconds (0.5));
                        serverApp.Stop (Seconds (3.0));

                  OnOffHelper onoff ("ns3::TcpSocketFactory", Ipv4Address::GetAny ());
                  onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
                  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
                  onoff.SetAttribute ("PacketSize", UintegerValue (payloadSize));
                  onoff.SetAttribute ("DataRate", DataRateValue (10000000)); //bit/s
                  AddressValue remoteAddress (InetSocketAddress (staInterfaces[0].GetAddress (0), port));
                  onoff.SetAttribute ("Remote", remoteAddress);
                  ApplicationContainer clientApp = onoff.Install (backboneNodes.Get (0));
     

  ////////////////////

  wifiPhy.EnablePcap ("tcp", apDevices[0]);
  wifiPhy.EnablePcap ("tcp", apDevices[1]);
   
  if (writeMobility)
    {
      AsciiTraceHelper ascii;
      MobilityHelper::EnableAsciiAll (ascii.CreateFileStream ("tcp.mob"));
    }

AnimationInterface anim ("tcp.xml");
  anim.EnablePacketMetadata (); // Optional
  anim.EnableIpv4L3ProtocolCounters (Seconds (0), Seconds (15)); // Optional


  Simulator::Stop (Seconds (5.0));
  Simulator::Run ();
  Simulator::Destroy ();
}
