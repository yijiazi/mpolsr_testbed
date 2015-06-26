#ifndef IPPACKET_H
#define IPPACKET_H

#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>

namespace sereadmo {
namespace net {

/**
*/
class IpPacket{
public:
  IpPacket(struct iphdr* iph) : iph_(iph){};
  virtual ~IpPacket();
  
  struct iphdr* getIpHeader(){return iph_;};
  char* getIpPayload() {return (iph_==NULL)?0:((char*)iph_+getIpHeaderSize());};
  
  int getIpHeaderSize() {return (iph_==NULL)?0:(iph_->ihl<<2);};
  int getIpTotalSize()  {return (iph_==NULL)?0:ntohs(iph_->tot_len);};
  int getIpPayloadSize()  {return getIpTotalSize()-getIpHeaderSize();};
  
  static void updateIpChecksum(struct iphdr* iph);
  static void updateUdpChecksum(struct iphdr* iph);
  static void dumpIpHeader(struct iphdr* iph);
  static void dumpUdpHeader(struct udphdr* udph);
  static void dumpHexData(char* data, long len);

  virtual void dump();
  
protected:
  struct iphdr* iph_;
};
}
} // End of Namespace sereadmo
#endif
