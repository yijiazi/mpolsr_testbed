#ifndef MPOLSR_PACKET_H
#define MPOLSR_PACKET_H

#include "ippacket.h"
#include "ser_path.h"

namespace sereadmo {
namespace net {

/**
	@author xle <xle@keosys.com>
*/
class MpOlsrPacket : public IpPacket
{
public:
  MpOlsrPacket(struct iphdr* iph);

  ~MpOlsrPacket();

  void setIpHeader(struct iphdr* iph);
  struct iphdr* getIpHeader();
  
  struct ser_header* getMpOlsrHeader();
  int getMpOlsrHeaderSize();

  char* getMpOlsrData();
  int   getMpOlsrDataSize();
  
  //int getIpHeaderSize();
  //int getIpTotalSize();

  ip_v4_addr* getMpOlsrPath();

  bool isEndOfPath();

  unsigned short getHopCount();
  ip_v4_addr getHopAddr(int nHop);
  
  ip_v4_addr getCurrentAddr();
  ip_v4_addr getNextAddr();
  ip_v4_addr getLastAddr();
  
  bool moveToNextHop();
  
  int checkNextHop(ip_v4_addr* nextHop);
  
  int updatePath();

  static bool addMpOlsrHeader(struct iphdr* iph, char* sData, long dataSize, 
                  char* newBuffer, long bufferSize);
  
  static bool replaceMpOlsrHeader(struct iphdr* iph, char* sData, long size, 
                  char* newBuffer, long bufferSize);
                  
  bool removeMpOlsrHeader(char* newBuffer, long bufferSize);

  //int replacePath(struct iphdr* iph, char* sData, unsigned long size, unsigned long initSize);
  
  virtual void dump();
  
  static void dumpMpOlsrHeader(struct ser_header* mpolsr);
  
protected:
  struct ser_header* ser_header_;
};

} // end of namespace sereadmo::net
} // end of namespace sereadmo
#endif
