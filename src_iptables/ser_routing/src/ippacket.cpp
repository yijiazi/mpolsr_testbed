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

#include <iostream>
#include <arpa/inet.h>
#include <netinet/udp.h>
#include <stdio.h>
#include "ippacket.h"

using namespace std;

namespace sereadmo {
namespace net {
//***********************************************************************
//***********************************************************************

IpPacket::~IpPacket()
{
}

//***********************************************************************
//***********************************************************************

void IpPacket::dumpIpHeader(struct iphdr* iph)
{
  struct in_addr addr;
  // IP Header
  cout << "IHL     : "<< (ushort)iph->ihl<< endl;
  cout << "VERSION : "<< (ushort)iph->version<< endl;
  cout << "TOS     : "<< (ushort)iph->tos<< endl;
  cout << "TOT_LEN : "<< (ushort)ntohs(iph->tot_len)<< endl;
  cout << "ID      : "<< (ushort)ntohs(iph->id)<< endl;
  cout << "FRAG OFF: "<< (ushort)ntohs(iph->frag_off)<< endl;
  cout << "TTL     : "<< (ushort)iph->ttl<< endl;
  cout << "PROTOCOL: "<< (ushort)iph->protocol<< endl;
  addr.s_addr = iph->saddr;
  cout << "SADDR   : "<< inet_ntoa(addr)<< endl;
  addr.s_addr = iph->daddr;
  cout << "DADDR   : "<< inet_ntoa(addr)<< endl;
}

//***********************************************************************
//***********************************************************************

void IpPacket::dumpUdpHeader(struct udphdr* udph)
{
  cout << "SOURCE  : "<< (ushort)ntohs(udph->source)<< endl;
  cout << "DEST    : "<< (ushort)ntohs(udph->dest)<< endl;
  cout << "LEN     : "<< (ushort)ntohs(udph->len)<< endl;
  cout << "CHECK   : "<< (ushort)ntohs(udph->check)<< endl;
}

//***********************************************************************
//***********************************************************************

void IpPacket::dumpHexData(char* data, long len)
{
  for (int i=0; i<len; i++)
  {
    if (i%16==0) cout << endl << "          ";
    printf("%02X ", (unsigned char)data[i]);
    cout.flush();
  }
  cout << endl;
}

//***********************************************************************
//***********************************************************************

void IpPacket::dump()
{
  struct in_addr addr;
  cout << "IP HEADER" << endl;
  cout << "=========" << endl;
  dumpIpHeader(iph_);
  
  if (iph_->protocol==IPPROTO_UDP)
  {
      cout << "UDP HEADER" << endl;
      cout << "=============" << endl;
      struct udphdr* udphdr=(struct udphdr*)getIpPayload();
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

//***********************************************************************
//***********************************************************************

static int SumWords(u_int16_t *buf, int nwords)
{
  register u_int32_t    sum = 0;

  while (nwords >= 16)
  {
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    sum += (u_int16_t) ntohs(*buf++);
    nwords -= 16;
  }
  while (nwords--)
    sum += (u_int16_t) ntohs(*buf++);
  return(sum);
}

//***********************************************************************
//***********************************************************************

/*static*/void IpPacket::updateIpChecksum(struct iphdr *iph)
{
  register u_int32_t    sum;

  iph->check = 0;
  sum = SumWords((u_int16_t *) iph, iph->ihl << 1);

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  sum = ~sum;

  iph->check = htons(sum);
}

//***********************************************************************
//***********************************************************************

/*static*/void IpPacket::updateUdpChecksum(struct iphdr *iph)
{
  struct udphdr *const udp = (struct udphdr *) ((long *) iph + iph->ihl);
  u_int32_t     sum;

  udp->check = 0;

  sum = SumWords((u_int16_t *) &iph->saddr, 4);
  sum += (u_int16_t) IPPROTO_UDP;
  sum += (u_int16_t) ntohs(udp->len);

  sum += SumWords((u_int16_t *) udp, ((u_int16_t) ntohs(udp->len)) >> 1);
  if (ntohs(udp->len) & 1)
    sum += (u_int16_t) (((u_char *) udp)[ntohs(udp->len) - 1] << 8);

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  sum = ~sum;

  udp->check = htons(sum);
}



} // End of namespace sereadmo::net
} // End of namespace sereadmo
