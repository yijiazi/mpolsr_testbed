/******************************************************************
* Copyright (C) 2010                                                     
*     by      Pascal Lesage (Keosys)                                       
*             Xavier Lecourtier (Keosys) xavier.lecourtier@keosys.com                    
*             Sylvain David (IRCCyN, University of Nantes, France) sylvain.david@polytech.univ-nantes.fr          
*             Jiazi Yi (IRCCyN, University of Nantes, France) jiazi.yi@.univ-nantes.fr 
*      Benoit Parrein (IRCCyN, University of Nantes, France) benoit.parrein@polytech.univ-nantes.fr
*
*     Members of SEREADMO (French Research Grant ANR-05-RNRT-02803)                                                         
*                                                                                                                                     
*     This program is distributed in the hope that it will be useful,                                                          
*     but WITHOUT ANY WARRANTY; without even the implied warranty of
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
****************************************************************
* Used and modified 2015
*   by      Benjamin Mollé (engineering student, Polytech Nantes, University of Nantes, France) benjamin.molle@gmail.com
*           Denis Souron (engineering student, Polytech Nantes, Université of Nantes, France) denis.souron@laposte.net
*******************************************************************/

#include <stdlib.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/udp.h>
extern "C" {
#include "common.h"
#include "ser_path.h"
}
#include "mpolsrpacket.h"


using namespace std;

namespace sereadmo {
namespace net {
//***********************************************************************
//***********************************************************************

MpOlsrPacket::MpOlsrPacket(struct iphdr* iph) : IpPacket(iph)
{
 // setIpHeader(iph);
  ser_header_ = NULL;
  if (iph_!=NULL)
  {
    //sanity check
    if(getIpPayloadSize()>=sizeof(struct ser_header))
    {
      ser_header_ = (struct ser_header*)((char*)iph + getIpHeaderSize());
    }
    else
    {
        MSG_FAILED("MpOlsrPacket ctor : Invalid payload size");
        return;
    }
  }
}


//***********************************************************************
//***********************************************************************

MpOlsrPacket::~MpOlsrPacket()
{
  iph_= NULL;
  ser_header_ = NULL;
}

void MpOlsrPacket::dump()
{
  IpPacket::dump();
  
  if (iph_->protocol==IPPROTO_MPOLSR)
  {
    struct ser_header *mpolsr = getMpOlsrHeader();
    cout << "MPOLSR HEADER" << endl;
    cout << "=============" << endl;
    dumpMpOlsrHeader(mpolsr);
    if (mpolsr->protocol==17)
    {
      cout << "UDP HEADER" << endl;
      cout << "=============" << endl;
      struct udphdr* udphdr=(struct udphdr*)getMpOlsrData();
      dumpUdpHeader(udphdr);
      int udpDataLen = ntohs(udphdr->len) - sizeof(struct udphdr);
      if (udpDataLen>0)
      {
        cout << "UDP DATA: " << udpDataLen << " bytes";
        char * udpData = (char*)udphdr + sizeof(struct udphdr);
        dumpHexData(udpData, udpDataLen);
        cout << "=============" << endl;
      }
    }
  }
  
  // UDP Header
}

void MpOlsrPacket::dumpMpOlsrHeader(struct ser_header* mpolsr)
{
    struct in_addr addr;
      // MPOLSR Header
    cout << "VERSION : "<< (ushort)mpolsr->version<< endl;
    cout << "PROTOCOL: "<< (ushort)mpolsr->protocol<< endl;
    cout << "HOPS    : "<< (ushort)ntohs(mpolsr->hops)<< endl;
    addr.s_addr =mpolsr->first_node;
    cout << "FIRST   : "<< inet_ntoa(addr)<< endl;
    cout << "CURRENT : "<< (ushort)ntohs(mpolsr->cur)<< endl;
    int nNbHops = ntohs(mpolsr->hops);
    int nCur = ntohs(mpolsr->cur);
    if (nNbHops>0)
    {
    
      unsigned long* tabAddr = (unsigned long*)((char*)mpolsr + sizeof(struct ser_header));
      for (int i=0; i<nNbHops; i++)
      {
        addr.s_addr =tabAddr[i];
        cout << "  HOP #" << i+1 << "  : "<< inet_ntoa(addr);
        if (i==nCur-1) cout << " <==";
        cout << endl;
      }
    }
}
//***********************************************************************
//***********************************************************************

void MpOlsrPacket::setIpHeader(struct iphdr* iph)
{
  ser_header_ = 0;
  iph_ = iph;
  if (iph!=NULL)
  {
    int payload_offset;  // MPOLSR payload offset (ie = path offset)
    int payload_size;    // MPOLSR payload size
    
    // calculation of MPOLSR payload position in ip buffer
    payload_offset = (iph->ihl<<2);
    payload_size = ntohs(iph->tot_len)-payload_offset;

    //sanity check
    if(payload_size<sizeof(struct ser_header))
    {
        MSG_FAILED("setIpHeader : Invalid payload size");
        iph_=NULL;
        return;
    }
    cout << "Payload offset=" << payload_offset << endl;
    ser_header_ = (struct ser_header*)((char*)iph + payload_offset);
  }
}

//***********************************************************************
//***********************************************************************

struct iphdr* MpOlsrPacket::getIpHeader()
{
   return iph_;
}
/*
//***********************************************************************
//***********************************************************************

int MpOlsrPacket::getIpHeaderSize()
{
   return (char*)ser_header_ - (char*)iph_;
}
*/

//***********************************************************************
//***********************************************************************

struct ser_header* MpOlsrPacket::getMpOlsrHeader()
{
   return ser_header_;
}

//***********************************************************************
//***********************************************************************

char* MpOlsrPacket::getMpOlsrData()
{
    if (ser_header_==NULL) 
      return NULL;
    else
      return ((char*)ser_header_) + getMpOlsrHeaderSize();
}

ip_v4_addr* MpOlsrPacket::getMpOlsrPath()
{
  return (ip_v4_addr*)((char*)ser_header_ + sizeof(struct ser_header));
}

//***********************************************************************
//***********************************************************************

int MpOlsrPacket::getMpOlsrDataSize()
{
    if (ser_header_==NULL) 
      return 0;
    else
      return getIpTotalSize() - getIpHeaderSize() - getMpOlsrHeaderSize();
}

//***********************************************************************
//***********************************************************************
/*
int MpOlsrPacket::getIpTotalSize()
{
  if (iph_==NULL) 
    return 0;
  else
    return ntohs(iph_->tot_len);
}
*/
//***********************************************************************
//***********************************************************************

int MpOlsrPacket::getMpOlsrHeaderSize()
{
    if (ser_header_==NULL) 
      return 0;
    else
      return (ntohs(ser_header_->hops)*ip_v4_size) + sizeof(struct ser_header);
}

//***********************************************************************
//***********************************************************************

bool MpOlsrPacket::moveToNextHop()
{
    if(ser_header_ == NULL)
    {
        MSG_FAILED("MpOlsrPacket::moveToNextHop :  invalid path information");
        return false;
    }
    
    int next  = ntohs(ser_header_->cur)+1;
    int hops = ntohs(ser_header_->hops);

    if(next > hops)
    {
        MSG_FAILED("MpOlsrPacket::moveToNextHop:  cannot move to next hop (next hop >= #hpos)");
        return false;  // can't get next hop if reach end of path
    }
    
    ser_header_->cur = htons(next);
    return true;
}

//***********************************************************************
//***********************************************************************

bool MpOlsrPacket::isEndOfPath()
{
    if(ser_header_ == NULL)
    {
        // current hop must be less than max
        MSG_FAILED("MpOlsrPacket::isEndOfPath :  invalid path information");
        return -1;
    }

    int                 cur;
    int                 hops;

    // check current hop index
    //------------------------
    cur = ntohs(ser_header_->cur);
    hops = ntohs(ser_header_->hops);

    if(cur > hops)
    {
        // current hop must be less than max
        MSG_FAILED("MpOlsrPacket::isEndOfPath :  invalid path information (cur hop > #hops)");
        return false;
    }

    if( hops != cur) // end of path only if current hop = total hops
    {
        //DBG_HK(3,"MpOlsrPacket::isEndOfPath : not end of path for datagram");
        return false;
    }

    if(cur<1)       // must have at least 1 hop
    {
        // current hop must be less than max
        MSG_FAILED("MpOlsrPacket::isEndOfPath :  invalid path information (cur < 1)");
        return false;
    }

    return true;  // hops == cur  => so addr must be equal
}

//***********************************************************************
//***********************************************************************

bool MpOlsrPacket::removeMpOlsrHeader(char* newBuffer, long bufferSize)
{
    if(ser_header_ == NULL)
    {
        // current hop must be less than max
        MSG_FAILED("MpOlsrPacket::removeMpOlsrHeader :  invalid path information");
        return false;
    }
    
    int newPacketSize = getIpTotalSize() - getMpOlsrHeaderSize();
    if (newPacketSize>bufferSize)
    {
        MSG_FAILED("MpOlsrPacket::removeMpOlsrHeader : buffer too small");
        return false;
    }
    
    // copy IP header first
    memcpy(newBuffer, iph_, getIpHeaderSize());
    
    // then copy MpOlsr data
    memcpy(newBuffer+getIpHeaderSize(), getMpOlsrData(), getMpOlsrDataSize());
    
    struct iphdr* iph = (struct iphdr*)newBuffer;
    //--------------------------------
    // update datagram header (IP+UDP)
    //--------------------------------
    iph->tot_len = ntohs(newPacketSize);
    
    // restore protocol number
    iph->protocol = ser_header_->protocol;
    iph->saddr = ser_header_->first_node;

    // update IP + UDP checksum
    updateUdpChecksum(iph);
    updateIpChecksum(iph);

    return true;
}

//***********************************************************************
//***********************************************************************
bool MpOlsrPacket::addMpOlsrHeader(struct iphdr* iph, char* sData, long dataSize, 
                                   char* newBuffer, long bufferSize)
{
    if(iph == NULL)
    {
        // current hop must be less than max
        MSG_FAILED("MpOlsrPacket::addMpOlsrHeader : invalid ip packet");
        return false;
    }
    
    IpPacket ip(iph);
    int newPacketSize = ip.getIpTotalSize() + dataSize;
    
    if (newPacketSize > bufferSize)
    {
        MSG_FAILED("MpOlsrPacket::addMpOlsrHeader : buffer too small");
        return false;
    }
    
    // Allocate new Ip packet with MpOlsr header
    //newIpPacket = new char[newPacketSize];
    
    // copy IP header first
    memcpy(newBuffer, iph, ip.getIpHeaderSize());
    
    // insert MpOlsr data
    memcpy(newBuffer + ip.getIpHeaderSize(), sData, dataSize);
    
    // then copy remaining data
    memcpy(newBuffer + ip.getIpHeaderSize() + dataSize, ip.getIpPayload(), ip.getIpPayloadSize());
    
    struct iphdr* iphNew = (struct iphdr*)newBuffer;
    //--------------------------------
    // update datagram header (IP+UDP)
    //--------------------------------
    iphNew->tot_len = ntohs(newPacketSize);
    
    // restore protocol number
    iphNew->protocol = IPPROTO_MPOLSR;
    updateIpChecksum(iphNew);

    return true;
}
//***********************************************************************
//***********************************************************************
bool MpOlsrPacket::replaceMpOlsrHeader(struct iphdr* iph, char* sData, long dataSize,
                                        char* newBuffer, long bufferSize)
{
    if(iph == NULL)
    {
        // current hop must be less than max
        MSG_FAILED("MpOlsrPacket::addMpOlsrHeader : invalid ip packet");
        return false;
    }
    
    MpOlsrPacket mpolsr(iph);
    int newPacketSize = mpolsr.getIpTotalSize() - mpolsr.getMpOlsrHeaderSize() + dataSize;
    
    if (newPacketSize>bufferSize)
    {
        MSG_FAILED("MpOlsrPacket::replaceMpOlsrHeader : buffer too small");
        return false;
    }
    
    // copy IP header first
    memcpy(newBuffer, iph, mpolsr.getIpHeaderSize());
    
    // insert MpOlsr data
    memcpy(newBuffer + mpolsr.getIpHeaderSize(), sData, dataSize);
    
    // then copy remaining data
    memcpy(newBuffer + mpolsr.getIpHeaderSize() + dataSize, mpolsr.getMpOlsrData(), mpolsr.getMpOlsrDataSize());
    
    struct iphdr* iphNew = (struct iphdr*)newBuffer;
    //--------------------------------
    // update datagram header (IP+UDP)
    //--------------------------------
    iphNew->tot_len = ntohs(newPacketSize);
    
    // protocol number should already be set to 99
    // iphNew->protocol = IPPROTO_MPOLSR;
    updateIpChecksum(iphNew);

    return true;
}

//***********************************************************************
//***********************************************************************
unsigned short MpOlsrPacket::getHopCount()
{
    return  (ser_header_!=NULL)?ntohs(ser_header_->hops):0;
}

//***********************************************************************
//***********************************************************************
ip_v4_addr MpOlsrPacket::getHopAddr(int hop)
{
    if(hop<=0 || hop>getHopCount())
    {
        MSG_FAILED("MpOlsrPacket::getHopAddr: index out of bounds)");
        return 0;  // can't get next hop if reach end of path
    }
    return getMpOlsrPath()[hop-1];
}

//***********************************************************************
//***********************************************************************
ip_v4_addr MpOlsrPacket::getCurrentAddr()
{
    int cur  = ntohs(ser_header_->cur);
    return getHopAddr(cur);
}
//***********************************************************************
//***********************************************************************
ip_v4_addr MpOlsrPacket::getNextAddr()
{
    int cur  = ntohs(ser_header_->cur);
    return getHopAddr(cur+1);
}

//***********************************************************************
//***********************************************************************
ip_v4_addr MpOlsrPacket::getLastAddr()
{
    return getHopAddr(getHopCount());
}

} // end of namespace sereadmo::net
} // end of namespace sereadmo

#ifdef TEST_UNITAIRE

#define PCKT_LEN        4096
#define MPOLSR_NB_HOPS  3
#define MPOLSR_HDR_LEN  (sizeof(struct ser_header) + MPOLSR_NB_HOPS*sizeof(unsigned long))
#define DATA_LEN        20

using namespace sereadmo::net;

int main()
{
  char buffer[PCKT_LEN];
  
  // Our own headers' structures
  struct iphdr *ip = (struct iphdr*) buffer;
  struct ser_header *mpolsr = (struct ser_header*) (buffer + sizeof(struct iphdr));
  struct udphdr *udp = (struct udphdr*) (buffer + sizeof(struct iphdr) + MPOLSR_HDR_LEN);
  
  // Source and destination addresses: IP and port
  struct sockaddr_in sin, din;
  
  memset(buffer, 0xFA, PCKT_LEN);  
  
  // Construct the IP header
  ip->ihl = 5;
  ip->version = 4;
  ip->tos = 16; // Low delay
  ip->tot_len = htons(sizeof(struct iphdr) + MPOLSR_HDR_LEN + sizeof(struct udphdr) + DATA_LEN);
  ip->id = 0;
  ip->frag_off = htons(IP_DF);
  ip->ttl = 64; // hops
  ip->protocol = IPPROTO_MPOLSR;
  // Source IP address, can use spoofed address here!!!
  ip->saddr= inet_addr("10.0.0.2");
  // The destination IP address
  ip->daddr = inet_addr("10.0.0.3");
  
  // Construct the MP-OLSR header
  mpolsr->version=1;
  mpolsr->protocol=17; // UDP
  mpolsr->empty=0;      // useless field
  mpolsr->hops=htons(MPOLSR_NB_HOPS);
  mpolsr->cur=htons(1);
  mpolsr->first_node=inet_addr("10.0.0.1");
  unsigned long* tabAddr = (unsigned long*)((char*)mpolsr + sizeof(struct ser_header));
  tabAddr[0] = inet_addr("10.0.0.2");
  tabAddr[1] = inet_addr("10.0.0.3");
  tabAddr[2] = inet_addr("10.0.0.4");
  
  
  // Fabricate the UDP header. Source port number, redundant
  udp->source = htons(32768);
  // Destination port number
  udp->dest = htons(104);
  udp->len = htons(sizeof(struct udphdr) + DATA_LEN);
  udp->check = 0;
  
  cout << (unsigned long) ip << endl;
  MpOlsrPacket packet(ip);
  packet.dump();
  
  
  unsigned long addr;
  addr = packet.getCurrentAddr();
  if (addr>0)
  {
    cout << "Current adress is : " << inet_ntoa(*(struct in_addr*)&addr) << endl;
  }
  addr = packet.getNextAddr();
  if (addr>0)
  {
    cout << "Next adress is : " << inet_ntoa(*(struct in_addr*)&addr) << endl;
  }
  else
  {
    cerr << "Couldn't get next node address" << endl;
  }
  if (packet.moveToNextHop())
    packet.dump();
    
  cout << "End of path: " << packet.isEndOfPath() << endl;
  if (packet.moveToNextHop())
    packet.dump();
  cout << "End of path: " << packet.isEndOfPath() << endl;
  if (packet.moveToNextHop())
    packet.dump();
  
  cout<<"Hop count: "<< packet.getHopCount()<<endl;
  return 0;
}
#endif
