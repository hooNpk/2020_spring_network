/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas
 *
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
 * Authors: Justin P. Rohrer, Truc Anh N. Nguyen <annguyen@ittc.ku.edu>, Siddharth Gangadhar <siddharth@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 *
 * “TCP Westwood(+) Protocol Implementation in ns-3”
 * Siddharth Gangadhar, Trúc Anh Ngọc Nguyễn , Greeshma Umapathi, and James P.G. Sterbenz,
 * ICST SIMUTools Workshop on ns-3 (WNS3), Cannes, France, March 2013
 */

#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("FinalProject");

int main(int argc, char *argv[])
{
  /* NOTICE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   * You should use following logs for only debugging. Please disable all logs when submit!!!!
   * For printing out throughput and propagation delay, you should use tracing system, not log components.
   * LogComponentEnable("UdpServer", (LogLevel)(LOG_LEVEL_ALL|LOG_PREFIX_NODE|LOG_PREFIX_TIME));
   * LogComponentEnable("UdpClient", (LogLevel)(LOG_LEVEL_ALL|LOG_PREFIX_NODE|LOG_PREFIX_TIME));
   * LogComponentEnable("BulkSendApplication", (LogLevel)(LOG_LEVEL_ALL|LOG_PREFIX_NODE|LOG_PREFIX_TIME));
   * LogComponentEnable("PacketSink", (LogLevel)(LOG_LEVEL_ALL|LOG_PREFIX_NODE|LOG_PREFIX_TIME));
   * ...
   */
  LogComponentEnable("FinalProject", LOG_LEVEL_INFO);
  //LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

  string topo, flow;
  uint32_t simTime = 100;
  CommandLine cmd;
  cmd.AddValue("topo_file", "The name of topology configuration file", topo);
  cmd.AddValue("flow_file", "The name of flow configuration file", flow);
  cmd.AddValue("sim_time", "Simulation Time", simTime);
  cmd.Parse(argc, argv);

  //TCP Configuration --> Do not modify
  Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 21));
  Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 21));
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1000));
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TypeId::LookupByName("ns3::TcpNewReno")));

  /*Set Topology*/
  std::ifstream topof;
  uint32_t node_num, switch_num, link_num;
  topof.open(topo.c_str());
  topof >> node_num >> switch_num >> link_num;

  NodeContainer n;
  n.Create(node_num);

  //To distinguish switch nodes from host nodes
  std::vector<uint32_t> node_type(node_num, 0);
  for(uint32_t i = 0; i < switch_num; i++)
  {
    uint32_t sid;
    topof >> sid;
    node_type[sid] = 1;
  }

  //Set initial parameters for host or switch nodes
  //You don't have to use this statement.(Use only if necessary)
  /*
  for(uint32_t i = 0; i < node_num; i++)
  {
    if(node_type[i] == 0)
    {
      Host Settings...
    }
    else
    {
      Swtich Settings...
    }
  }
  */

  InternetStackHelper internet;
  internet.Install(n);

  Ipv4AddressHelper address;
  address.SetBase("10.0.0.0", "255.255.255.0");

  //Assume that each host nodes (server/client nodes) connects to a single switch in this project
  std::vector<Ipv4Address> server_addresses(node_num, Ipv4Address());

  for(uint32_t i=0;i<link_num;i++)
  {
    uint32_t src, dst;
    string bandwidth, link_delay;
    topof >> src >> dst >> bandwidth >> link_delay;

    DataRate rate(bandwidth);
    Time delay(link_delay);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue(bandwidth));
    p2p.SetChannelAttribute("Delay", StringValue(link_delay));
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("50p")));

    NetDeviceContainer devices;
    devices = p2p.Install(n.Get(src), n.Get(dst));

    /* Install Traffic Controller
     * Do not disable traffic controller.
     * Packets are enqueued in a new efficient queue instead of Netdevice's droptail queue.
     * But if you want to use different Queue Disc like CoDel and RED, you can use it!
     * Max Size should be the same as 50p
     * You should write the reason why use other queue disc than PfifoFastQueueDisc in report and presentation ppt material.
     */
    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::PfifoFastQueueDisc", "MaxSize", QueueSizeValue(QueueSize("50p"))); //ns3 default queue disc
    //tch.SetRootQueueDisc("ns3::CoDelQueueDisc", "MaxSize", QueueSizeValue(QueueSize("50p")); //CoDel queue disc
    //tch.SetRootQueueDisc("ns3::RedQueueDisc", "MaxSize", QueueSizeValue(QueueSize("50p")); //RED queue disc
    //tch.SetRootQueueDisc("ns3::...")
    tch.Install(devices);

    Ipv4InterfaceContainer ipv4 = address.Assign(devices);
    address.NewNetwork();

    //Store addresses of servers for installing applications later.
    if(node_type[src] == 0 || node_type[dst] == 0)
    {
      uint32_t host_index = node_type[src] ? 1 : 0;
      uint32_t host_id = node_type[src] ? dst : src;
      server_addresses[host_id] = ipv4.GetAddress(host_index);
    }
  }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  /*Set Flows*/
  std::ifstream flowf;
  uint32_t flow_num;
  flowf.open(flow.c_str());
  flowf >> flow_num;

  for(uint32_t i=0;i<flow_num;i++)
  {
    string protocol;
    uint32_t src, dst, port, maxPacketCount;
    double startTime;

    flowf >> protocol >> src >> dst >> port >> maxPacketCount >> startTime;

    //Do not modify parameters of TCP
    if(protocol == "TCP")
    {
      Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
      PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);
      sinkHelper.SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
      ApplicationContainer sinkApp = sinkHelper.Install(n.Get(dst));
      sinkApp.Start(Seconds(startTime));

      AddressValue remoteAddress(InetSocketAddress(server_addresses[dst], port));
      BulkSendHelper ftp("ns3::TcpSocketFactory", Address());
      ftp.SetAttribute("Remote", remoteAddress);
      ftp.SetAttribute("SendSize", UintegerValue(1000));
      ftp.SetAttribute("MaxBytes", UintegerValue(maxPacketCount * 1000));
      ApplicationContainer sourceApp = ftp.Install(n.Get(src));
      sourceApp.Start(Seconds(startTime));
    }
    //You can add/remove/change parameters of UDP
    else
    {
      UdpServerHelper server(port);
      ApplicationContainer serverApp = server.Install(n.Get(dst));
      serverApp.Start(Seconds(startTime));
      serverApp.Stop(Seconds(simTime));

      UdpClientHelper client(server_addresses[dst], port);
      client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
      //client.SetAttribute("Interval", TimeValue(interPacketInterval)); Managed by application-level congestion controller
      client.SetAttribute("PacketSize", UintegerValue(1000)); //Do not modify
      ApplicationContainer clientApp = client.Install(n.Get(src));
      clientApp.Start(Seconds(startTime));
    }
  }

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  Simulator::Destroy();
  return 0;
}
