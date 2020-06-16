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

/*
class FlowRTT{
  public:
    uint32_t min;
    uint32_t max;
    uint32_t avg;
    uint32_t nine;
};
std::map<int, FlowRTT> RTTHistory;
static void RttTrace(std::string Context, uint32_t min, uint32_t max, uint32_t avg, uint32_t nine){
std::cout << "Tracing is start\n" << std::endl;
  int flowNumber = std::stoi(Context);
  RTTHistory[flowNumber].min = min;
  RTTHistory[flowNumber].max = max;
  RTTHistory[flowNumber].avg = avg;
  RTTHistory[flowNumber].nine = nine;
  std::cout << "Tracing is end\n" << std::endl;
}
*/

/*
class FlowRTT{
  public:
    Ptr<const Packet> packet;
};
std::map<Ptr<const Packet>, FlowRTT> RTTHistory;
static void RttTrace(std::string Context, Ptr<const Packet> p){
  int flowNumber = std::stoi(Context);
  std::cout << "flowNumber : " << flowNumber << std::endl;
  std::cout << "Tracing is start\n" << std::endl;
}
*/

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
  LogComponentEnable("UdpServer", LOG_LEVEL_INFO);
  LogComponentEnable("FinalProject", LOG_LEVEL_INFO);

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

  std::cout << "simTime   :  " << simTime << std::endl;
  /*Set Topology*/
  std::ifstream topof;
  uint32_t node_num, switch_num, link_num;
  topof.open(topo.c_str());
  topof >> node_num >> switch_num >> link_num;

  NodeContainer n;
  n.Create(node_num);

  std::cout << "Create node container" << "Node_NUM : " << node_num <<std::endl;


  //To distinguish switch nodes from host nodes
  std::vector<uint32_t> node_type(node_num, 10);
  for(uint32_t i = 0; i < switch_num; i++)
  {
    //std::cout << "1" << std::endl;
    uint32_t sid;
    //std::cout << "2" << std::endl;
    topof >> sid;
    //std::cout << "3" << std::endl;
    node_type[sid] = 1;
    //std::cout << "4" << std::endl;
  }

  std::cout << "Switch Nodes Setting" << std::endl;
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
    std::cout << " src : " << src << " dst : " << dst << std::endl;

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
  std::cout << "Topology Setting Succesfully" << std::endl;

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  /*Set Flows*/
  std::ifstream flowf;
  uint32_t flow_num=0;

  flowf.open(flow.c_str());
  flowf >> flow_num;

  for(uint32_t i=0;i<flow_num;i++)
  {
    string protocol;
    uint32_t src, dst, port, maxPacketCount;
    double startTime;

    flowf >> protocol >> src >> dst >> port >> maxPacketCount >> startTime;

    std::cout << "File read" << "src : " << src << " dst : " << dst << std::endl;
    std::cout << "File read" << "protocol : " << protocol << " port : " << port << std::endl;

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
      //std::cout << "UDP Server generation Succesfully" << std::endl;
      serverApp.Start(Seconds(startTime));

      //std::cout << "startTime : " << startTime <<std::endl;
      //std::cout << "socket : " << serverApp.Get(0)-> GetObject<UdpServer>()->GetSocket() << std::endl;
    //  Ptr<Socket> socket = serverApp.Get(0) -> GetObject<UdpServer>()->GetSocket();

      //std::cout << "2" << std::endl;
      //serverApp.Get(0)->TraceConnect("Rx", std::to_string(i), MakeCallback(&RttTrace));
      //socket->TraceConnect("Rx", std::to_string(i), MakeCallback(&RttTrace));
      //std::cout << "3" << std::endl;


      UdpClientHelper client(server_addresses[dst], port);
      client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
      //client.SetAttribute("Interval", TimeValue(interPacketInterval)); Managed by application-level congestion controller
      client.SetAttribute("PacketSize", UintegerValue(1000)); //Do not modify
      std::cout << "5" << std::endl;
      ApplicationContainer clientApp = client.Install(n.Get(src));
      clientApp.Start(Seconds(startTime));
    }
  }


  std::cout << "Simulator Stop" << std::endl;
  Simulator::Stop(Seconds(simTime));
  std::cout << "Simulator Run" << std::endl;
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
