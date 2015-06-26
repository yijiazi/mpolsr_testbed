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
*****************************************************************

*
* Add, retrieve and remove datagram path in UDP payload
*
* Datagram parts :
*  - without path : IP header + UDP header + UDP payload
*  - with path    : IP header + path + UDP header + UDP payload
*************************************************************************
* Changes :
* --------
*
*************************************************************************/
#include <limits.h>
#include <string.h>
#include <stdio.h>
//#include <linux/in.h>
//#include <linux/ip.h>
//#include <linux/udp.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <linux/netfilter_ipv4.h>
#include "common.h"
#include "ser_path.h"

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
   
/* 
Various checksum routines taken from TCPDUMP and http://www.bytebot.net/rrjcode/ratelimit-0.1/
*/
void skb_updateChecksumIP(struct ip *ip)
{
  register u_int32_t    sum;

  ip->ip_sum = 0;
  sum = SumWords((u_int16_t *) ip, ip->ip_hl << 1);

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  sum = ~sum;

  ip->ip_sum = htons(sum);
}


void UdpChecksum(struct ip *ip)
{
  struct udphdr *const udp = (struct udphdr *) ((long *) ip + ip->ip_hl);
  u_int32_t     sum;

  udp->check = 0;

  sum = SumWords((u_int16_t *) &ip->ip_src, 4);
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

//***********************************************************************
//***********************************************************************
/*
void skb_updateChecksumIP(struct iphdr* iph)
{
    //update IP header
    iph->check = 0;
    iph->check = ip_fast_csum((unsigned char *)iph, iph->ihl);
}
*/

//***********************************************************************
//***********************************************************************

/*
void skb_updateChecksumIP_UDP(struct iphdr *iph)
{
    struct udphdr *uh;
    unsigned long len;

    // retrieve UDP header
    uh = (struct udphdr*)(iph+(iph->ihl<<2));

    //update IP header
    iph->check = 0;
    iph->check = ip_fast_csum((unsigned char *)iph, iph->ihl);

    // update UDP header
    len = ntohs(iph->tot_len) - (iph->ihl<<2);
    long skb_csum = csum_partial((char*)uh + sizeof(struct udphdr),len-sizeof(struct udphdr),0);

    //skb_checksum(skb, offset, skb->len-offset, 0);

    uh->check =0;
    uh->check = csum_tcpudp_magic(iph->saddr,iph->daddr,len,IPPROTO_UDP,
                csum_partial((char*)uh,sizeof(struct udphdr),skb_csum));


}
*/

void skb_updateChecksumIP_UDP(struct iphdr *iph)
{
  skb_updateChecksumIP((struct ip*)iph);
  UdpChecksum((struct ip*)iph);
}

//***********************************************************************
//***********************************************************************


int skb_addPath(struct iphdr *iph, char* sData, unsigned long size)
{
    int   payload_offset; // UDP payload  (without path)
    int   payload_size;  // UDP size     (without path)
    char* payload;
    char* payload_end;
    char* buffer_end; 
    int i;

    //--------------------------------------------------------------------
    // add free space between UDP header and UDP payload for datagram path
    //--------------------------------------------------------------------

    // calculation of UDP payload position in sk_buff data
    payload_offset = (iph->ihl<<2);
    payload_size = htons(iph->tot_len) - (iph->ihl<<2);

/*
    //sanity check
    if(((payload_offset + payload_size)> skb->len) || (payload_size < 0) ||
        ((payload_offset + payload_size + size )> skb->len))
    {
        MSG_FAILED("ADD PATH : Invalid payload calculation");
        return -1;
    }
*/
    // get handler to UDP payload
    /*
    payload = skb_header_pointer(skb,payload_offset,payload_size, NULL);
    if(payload == NULL)
    {
        MSG_FAILED("ADD PATH : Failed to get payload data handler");
        return -1;
    }*/
    payload = (char*)iph + payload_offset;

    // move down UDP payload
    payload_end = payload + payload_size -1;
    buffer_end = payload + payload_size + size -1;
    for(i=0; i< payload_size;i++)
        *(buffer_end--) = *(payload_end--);

    //---------
    // add path
    //---------
    memcpy(payload,sData,size);

    //----------------------------
    // update datagram header (IP)
    //----------------------------
    iph->tot_len = ntohs(htons(iph->tot_len)+size);

    return 0;
}

//***********************************************************************
//***********************************************************************


int skb_replacePath(struct iphdr *iph, char* sData, unsigned long size, unsigned long initSize)
{
    int   old_path_offset;
    int   request_buf_size;
    int   payload_size;
    char* old_path;
    char* payload_end;
    char* buffer_end;
    char* payload_begin;
    char* buffer_begin;
    int i;

    // calculation of UDP payload position in sk_buff data
    old_path_offset = (iph->ihl<<2) + sizeof(struct ser_header);
    request_buf_size = htons(iph->tot_len) - (iph->ihl<<2) - sizeof(struct ser_header);


/*
    //sanity check
    // TODO
*/

    // get handler to UDP payload
    old_path = (char*)iph+old_path_offset;
    /*if(old_path == NULL)
    {
        MSG_FAILED("REPLACE PATH : Failed to get old path data handler");
        return -1;
    }*/

    if(size > initSize)
    {
        // move down payload
        payload_end = old_path + request_buf_size -1;
        buffer_end = old_path + request_buf_size + (size-initSize) -1;
        for(i=0; i< request_buf_size;i++)
            *(buffer_end--) = *(payload_end--);
    }
    else if(size < initSize)
    {
        // move up payload
        payload_begin = old_path + (initSize - size);
        buffer_begin = old_path;
        payload_size = request_buf_size + size - initSize;

        for(i=0; i< payload_size;i++)
            *(buffer_begin++) = *(payload_begin++);

    }

    //---------
    // add path
    //---------
    memcpy(old_path,sData,size);

    //----------------------------
    // update datagram header (IP)
    //----------------------------
    iph->tot_len = ntohs(htons(iph->tot_len)+ (size-initSize));

    return 0;
}

//***********************************************************************
//***********************************************************************

int skb_getPath(struct iphdr* iph, char* sData, unsigned long* size, struct ser_header* pheader)
{
    int payload_offset;  // path + UDP payload offset (ie = path offset)
    int payload_size;    // path + UDP payload size
    char* payload;
    int hops;

    //---------------------------------
    // retrieve UDP payload information
    //---------------------------------

    // calculation of UDP payload position in sk_buff data
    payload_offset = (iph->ihl<<2);
    payload_size = ntohs(iph->tot_len)-(iph->ihl<<2);

    //sanity check
    if(((payload_offset + payload_size) > ntohs(iph->tot_len)) || (payload_size < 0))
    {
        MSG_FAILED("GET PATH : Invalid payload calculation");
        return -1;
    }

    // get handler to path + UDP payload
    payload = (char*)iph + payload_offset;
    if(payload == NULL)
    {
        MSG_FAILED("GET PATH : Failed to get payload data handler");
        return -1;
    }

    //--------------
    // retrieve path
    //--------------

    // get nb hops
    memcpy(pheader,payload,sizeof(struct ser_header));

    //get path
    hops = ntohs(pheader->hops);
    *size = (hops*ip_v4_size) + sizeof(struct ser_header);
    memcpy(sData,payload,*size);

    return 0;
}

//***********************************************************************
//***********************************************************************

int skb_updatePath(struct iphdr* iph, char* sData, unsigned long size)
{
    int payload_offset;  // path + UDP payload offset (ie = path offset)
    int payload_size;   // path + UDP payload size
    char* payload;

    //---------------------------------
    // retrieve UDP payload information
    //---------------------------------
    // calculation of UDP payload position in sk_buff data
    payload_offset = iph->ihl<<2;
    payload_size = ntohs(iph->tot_len)-(iph->ihl<<2);

    //sanity check
    /*
    if(((payload_offset + payload_size) > ntohs(iph->tot_len)) || (payload_size < 0))
    {
        MSG_FAILED("UPDATE PATH : Invalid payload calculation");
        return -1;
    }*/

    // get handler to path + UDP payload
    //payload = skb_header_pointer(skb,payload_offset,payload_size, NULL);
    payload = (char*)iph + payload_offset;
    if(payload == NULL)
    {
        MSG_FAILED("UPDATE PATH : Failed to get payload data handler");
        return -1;
    }

    //----------------------------
    // update current hop position
    //----------------------------
    memcpy(payload,sData,size);

    return 0;
}



//***********************************************************************
//***********************************************************************


int skb_removePath(struct iphdr* iph,unsigned long size)
{
    int udp_payload_offset;  // udp payload offset
    int ser_payload_offset;  // path + udp payload offset (ie = path offset)
    int udp_payload_size;    // udp payload size
    int ser_payload_size;    // path + udp payload size

    char* ser_payload;
    char* udp_payload;
    int i;


    //---------------------------------
    // retrieve UDP payload information
    //---------------------------------

    // calculation of UDP payload and path position in sk_buff data
    ser_payload_offset = iph->ihl<<2;
    udp_payload_offset = ser_payload_offset+size;
    ser_payload_size   = ntohs(iph->tot_len)-(iph->ihl<<2);
    udp_payload_size   = ser_payload_size - size;

    //sanity check
    if(((ser_payload_offset + ser_payload_size) > ntohs(iph->tot_len)) || 
        (udp_payload_offset > ntohs(iph->tot_len)) || (udp_payload_size < 0)  ||
        (ser_payload_size < 0))
    {
        MSG_FAILED("REMOVE PATH : Invalid payload calculation");
        return -1;
    }

    // get handler to path + UDP payload
    //ser_payload = skb_header_pointer(skb,ser_payload_offset,ser_payload_size, NULL);
    ser_payload = (char*)iph + ser_payload_offset;
    if(ser_payload == NULL)
    {
        MSG_FAILED("REMOVE PATH : Failed to get payload data handler");
        return -1;
    }

    //--------------------
    // move up UDP payload
    //--------------------
    udp_payload = ser_payload + size;
    for(i=0; i< udp_payload_size;i++)
        *(ser_payload++) = *(udp_payload++);

    //--------------------------------
    // update datagram header (IP+UDP)
    //--------------------------------
    iph->tot_len = ntohs(htons(iph->tot_len)-size);

    // remove useless bytes
    //skb_trim(skb,skb->len-size);

    return 0;
}
