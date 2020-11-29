# /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
# /*
#  * This program is free software; you can redistribute it and/or modify
#  * it under the terms of the GNU General Public License version 2 as
#  * published by the Free Software Foundation;
#  *
#  * This program is distributed in the hope that it will be useful,
#  * but WITHOUT ANY WARRANTY; without even the implied warranty of
#  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  * GNU General Public License for more details.
#  *
#  * You should have received a copy of the GNU General Public License
#  * along with this program; if not, write to the Free Software
#  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#  */

# Default network topology includes some number of AP nodes specified by
# the variable nWifis (defaults to two).  Off of each AP node, there are some
# number of STA nodes specified by the variable nStas (defaults to two).
# Each AP talks to its associated STA nodes.  There are bridge net devices
# on each AP node that bridge the whole thing into one network.

#      +-----+      +-----+            +-----+      +-----+
#      | STA |      | STA |            | STA |      | STA |
#      +-----+      +-----+            +-----+      +-----+
#    192.168.0.2  192.168.0.3        192.168.0.5  192.168.0.6
#      --------     --------           --------     --------
#      WIFI STA     WIFI STA           WIFI STA     WIFI STA
#      --------     --------           --------     --------
#        ((*))       ((*))       |      ((*))        ((*))
#                                |
#              ((*))             |             ((*))
#             -------                         -------
#             WIFI AP   CSMA ========= CSMA   WIFI AP
#             -------   ----           ----   -------
#             ##############           ##############
#                 BRIDGE                   BRIDGE
#             ##############           ##############
#               192.168.0.1              192.168.0.4
#               +---------+              +---------+
#               | AP Node |              | AP Node |
#               +---------+              +---------+

import ns.core
import ns.internet
import ns.network
import ns.applications
import ns.wifi
import ns.mobility


def main(argv):
    cmd = ns.core.CommandLine ()
    
    cmd.nWifis = 2
    cmd.nStas = 2
    cmd.sendIp = "True"
    cmd.writeMobility = "False"
    cmd.AddValue ("nWifis", "Number of wifi networks")
    cmd.AddValue ("nStas", "Number of stations per wifi network")
    cmd.AddValue ("SendIp", "Send Ipv4 or raw packets")
    cmd.AddValue ("writeMobility", "Write mobility trace")
    cmd.Parse (sys.argv)

    nWifis=cmd.nWifis
    nStas=cmd.nStas
    sendIp=cmd.sendIp
    writeMobility=cmd.writeMobility


    backboneNodes=ns.network.NodeContainer()
    backboneDevices=ns.network.NetDeviceContainer()

    backboneInterfaces=ns.network.Ipv4InterfaceContainer
#       std::vector<NodeContainer> staNodes;
#   std::vector<NetDeviceContainer> staDevices;
#   std::vector<NetDeviceContainer> apDevices;
#   std::vector<Ipv4InterfaceContainer> staInterfaces;
#   std::vector<Ipv4InterfaceContainer> apInterfaces;
    staNodes=[]
    staDevices=[]
    apDevices=[]
    staInterfaces=[]
    apInterfaces=[]

    

    stack=ns.internet.InternetStackHelper ()
    csma = ns.csma.CsmaHelper ()
    ip = ns.internet.Ipv4AddressHelper ()
    ip.SetBase (ns.network.Ipv4Address ("192.168.0.0"), ns.network.Ipv4Mask ("255.255.255.0"))

     
    backboneNodes.Create (nWifis)
    stack.Install (backboneNodes)
    backboneDevices = csma.Install (backboneNodes)

    wifiX = 0.0


    wifiPhy = ns.wifi.YansWifiPhyHelper.Default ()
    wifiPhy.SetPcapDataLinkType (ns.wifi.YansWifiPhyHelper.DLT_IEEE802_11_RADIO)
    #   phy.SetChannel (channel.Create ());

    for i in range(0,nWifis):


        # calculate ssid for wifi subnetwork
        # std::ostringstream oss;
        # oss << "wifi-default-" << i;
        # Ssid ssid = Ssid (oss.str ());

        oss="wifi-default-" +i
        ssid = ns.wifi.Ssid (oss)

        sta=ns.network.NodeContainer()
        staDev=ns.network.NetDeviceContainer()
        apDev=ns.network.NetDeviceContainer()
        staInterface=ns.network.Ipv4InterfaceContainer()
        apInterface=ns.network.Ipv4InterfaceContainer()
        mobility = ns.mobility.MobilityHelper ()
        bridge = ns.mobility.BridgeHelper ()
        wifi=ns.wifi.WifiHelper()
        wifiMac=ns.wifi.WifiMacHelper()
        wifiChannel = ns.wifi.YansWifiPhyHelper.Default ()
        wifiPhy.SetChannel (wifiChannel.Create ())

        sta.Create (nStas)
        mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", ns.core.DoubleValue (wifiX),
                                     "MinY", ns.core.DoubleValue (0.0),
                                     "DeltaX", ns.core.DoubleValue (5.0),
                                     "DeltaY", ns.core.DoubleValue (5.0),
                                     "GridWidth", ns.core.UintegerValue (1),
                                     "LayoutType", ns.core.StringValue ("RowFirst"))


        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel")
        mobility.Install (backboneNodes.Get (i))
        wifiMac.SetType ("ns3::ApWifiMac","Ssid", ns.wifi.SsidValue (ssid))
        apDev = wifi.Install (wifiPhy, wifiMac, backboneNodes.Get (i))
       
        bridgeDev=ns.network.NetDeviceContainer()
        bridgeDev = bridge.Install (backboneNodes.Get (i), ns.network.NetDeviceContainer (apDev, backboneDevices.Get (i)))

        #assign AP IP address to bridge, not wifi
        apInterface = ip.Assign (bridgeDev)

        stack.Install (sta)
        mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                 "Mode", ns.core.StringValue ("Time"),
                                 "Time", ns.core.StringValue ("2s"),
                                 "Speed", ns.core.StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                                 "Bounds", ns.core.RectangleValue (ns.mobility.Rectangle (wifiX, wifiX + 5.0,0.0, (nStas + 1) * 5.0)))
        mobility.Install (sta)
        wifiMac.SetType ("ns3::StaWifiMac","Ssid", ns.wifi.SsidValue (ssid))
        staDev = wifi.Install (wifiPhy, wifiMac, sta)
        staInterface = ip.Assign (staDev)

        #save everything in containers.
        staNodes.append(sta)      
        apDevices.append(apDev)
        apInterfaces.append(apInterface)
        staDevices.append(staDev)
        staInterfaces.append(staInterface)
        # staNodes.push_back (sta);
        # apDevices.push_back (apDev);
        # apInterfaces.push_back (apInterface);
        # staDevices.push_back (staDev);
        # staInterfaces.push_back (staInterface);

        wifiX += 20.0
    
    if sendIp:
        protocol="ns3::TcpStartTimeSocketFactory"
        dest = ns.network.InetSocketAddress (staInterfaces[1].GetAddress (1), 1025)
        

   
    # else :
    #     PacketSocketAddress tmp
    #     tmp.SetSingleDevice (staDevices[0].Get (0)->GetIfIndex ())
    #     tmp.SetPhysicalAddress (staDevices[1].Get (0)->GetAddress ())
    #     tmp.SetProtocol (0x807)
    #     dest = tmp
    #      protocol = "ns3::PacketSocketFactory"






    #  OnOffHelper onoff = OnOffHelper (protocol, dest);
    #  onoff.SetConstantRate (DataRate ("500kb/s"));
    #  ApplicationContainer apps = onoff.Install (staNodes[0].Get (0));
    #  apps.Start (Seconds (0.5));
    #  apps.Stop (Seconds (9.0));
    onoff = ns.applications.OnOffHelper (protocol,dest)
    onoff.SetAttribute ("DataRate", ns.network.DataRateValue (ns.network.DataRate (500))) # bit/s(check)
    apps = onoff.Install (staNodes[0].Get (0))
    apps.Start (ns.core.Seconds (0.5))
    apps.Stop (ns.core.Seconds (9.0))

    wifiPhy.EnablePcap ("base_wifi-wired-bridging", apDevices[0])
    wifiPhy.EnablePcap ("base_wifi-wired-bridging", apDevices[1])

    if writeMobility:
        ascii = ns.network.AsciiTraceHelper()
        mobility.EnablePcapAll ("wifi-wired-bridging.moby", True)
         # pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send-py.tr"))

    # AnimationInterface anim ("base.xml")
    # anim.EnablePacketMetadata (); # Optional
    # anim.EnableIpv4L3ProtocolCounters (Seconds (0), Seconds (15)) #Optional

    ns.core.Simulator.Stop (ns.core.Seconds (15.0))
    ns.core.Simulator.Run ()
    ns.core.Simulator.Destroy ()
    

if __name__ == '__main__':
    import sys
    main (sys.argv)





















    # staDevices = ns.network.NetDeviceContainer ()   
	# staDevices = wifi.Install (phy, mac, wifiStaNodes)