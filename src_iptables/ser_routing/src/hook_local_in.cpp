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

/************************************************************************
*             Projet SEREADMO - N° ANR-05-RNRT-028-01
*              Securite des Reseaux Ad hoc & Mojette
*************************************************************************
*
* AUTHORS    : P.Lesage <pascal.lesage@keosys.com>
* VERSION    : 1.0
*
=========================================================================
*
* 'Local-in' netfilter hook implementation
* This hook is used to process fragmented datagram not processed by
* 'pre-routing' hook and after netfilter merge them in only 1 skb
*
*************************************************************************
* Changes :
* --------
*
*************************************************************************/

#include <iostream>
#include "/usr/include/netinet/in.h"
#include <string.h>


extern "C" {
#include "common.h"
#include <linux/netfilter.h>        /* for NF_ACCEPT */
#include <linux/netfilter_ipv4.h> // for NF_IP_xxx
#include <libnetfilter_queue/libnetfilter_queue.h>
}
#include "ser_path.h"

#include "rawsocket.h"
#include "mpolsrpacket.h"

using namespace std;
using namespace sereadmo::net;

namespace sereadmo {
namespace nfhooks {

//***********************************************************************
//***********************************************************************
static RawSocket g_rawSocket;

static int hk_route_recovery_local_in(struct iphdr* iph, char *newpayload, u_int32_t& newpayloadlen)
{
    unsigned long size = MAX_MPOLSR_PATH;
    static char path[MAX_MPOLSR_PATH] __attribute__ ((aligned));
    ip_v4_addr srcAddr=0;
    ip_v4_addr dstAddr=0;
    ip_v4_addr nextHop=0;
    
    ser_header header;
    
    // retrieve path
    MpOlsrPacket packet(iph); 

    // get current node address
    srcAddr = iph->daddr;
    
    // retrieve last node address
    dstAddr = packet.getLastAddr();
    if (dstAddr<=0)
    {
        MSG_FAILED("LOCAL IN  : Failed to retrieve last node in data path for route recovery\n");
        return -1;
    }
    
    //-------------------
    // find datagram path
    //-------------------
    memcpy(&header,packet.getMpOlsrHeader(), sizeof(struct ser_header));
    header.first_node=srcAddr;
    
    if(pth_find_datagram_path(path,&size, srcAddr, dstAddr,&nextHop,&header) <0 )
    {
        // failed to find path
        MSG_FAILED("LOCAL IN  : failed to find path from %lX to %lX for route recovery\n",srcAddr, dstAddr);
        return -1;
    }

    if(size == 0)
        // datagram for localhost ??? => no path, no modification
        return -1;

    if (MpOlsrPacket::addMpOlsrHeader(iph, path, size, newpayload, newpayloadlen)==false)
    {
        MSG_FAILED("LOCAL IN  : Failed to add MpOlsr header\n");
        return -1;
    }
    struct iphdr* iphNew = (struct iphdr*)newpayload;
    IpPacket ip(iphNew);
    newpayloadlen=ip.getIpTotalSize();

    //------------------------------------
    // update protocol number and next hop
    //------------------------------------
    iph->protocol = IPPROTO_MPOLSR;
    iph->daddr = nextHop;
    iph->saddr = srcAddr;
    
    //----------------------------------
    // update checksum and accept datagram
    //----------------------------------
    IpPacket::updateIpChecksum(iphNew);
    
    return 1;
}


int hk_process_datagram_local_in(struct iphdr* iph, char *newpayload, u_int32_t& newpayloadlen)
{
    unsigned long size = 0;
    ip_v4_addr curAddr;
    ip_v4_addr nextAddr;
    ip_v4_addr srcAddr;
    int ret;
    
    // in 'local in' hook we process fragmented datagram
    // => it's to late to forward this datagram if local host,
    //    is not dst. => we must send an other with same data
    //newpayload = NULL;
    //newpayloadlen=0;

    // retrieve path
    MpOlsrPacket packet(iph); 

    if (packet.getMpOlsrHeader()==NULL)
        return -1;

    curAddr = packet.getCurrentAddr();
    if (curAddr<=0)
        return -1;

    if (pth_checkCurrentAddr(curAddr))
        return -1;

    //------------------------------------
    // check if local host is datagram dst
    //------------------------------------
    if(packet.isEndOfPath())
    {
        // datagram is for localhost
        // remove path from payload
        if (packet.removeMpOlsrHeader(newpayload, newpayloadlen)==false)
            return -1;

        newpayloadlen = packet.getIpTotalSize() - packet.getMpOlsrHeaderSize();

        return 0;
    }

    // local host is not the right dst => send new datagram
    srcAddr = iph->daddr;

    //-----------------------------
    // update datagram for next hop
    //-----------------------------
    nextAddr = packet.getNextAddr();
    if(nextAddr <= 0)
        // failed to get next hop
        return -1;
        
    ret = pth_checkNextHop(&nextAddr);
    if(ret < 0)
        // failed to get next hop
        return -1;

    if(ret > 0)
        // next hop not available => need route recovery
        return hk_route_recovery_local_in(iph, newpayload, newpayloadlen);

    if (packet.moveToNextHop()==false)
        // failed to update path information
        return -1;

    // update datagram dst
    iph->daddr = nextAddr;
    iph->saddr = srcAddr;

    //----------------------------------
    // update checksum 
    //----------------------------------
    packet.updateIpChecksum(iph);
    
    //----------------------------------
    // As we come from LOCAL_IN hook, our datagram will not be 
    // automatically forwarded by the IP stack (it's too late, routing is done)
    // To forward the datagram, we reinject it via raw socket
    //----------------------------------
    g_rawSocket.sendTo(nextAddr, (unsigned char*)iph, packet.getIpTotalSize());
    
    return 1;
}



//***********************************************************************
//***********************************************************************


unsigned int hk_local_in_hook( struct nfq_q_handle *qh, struct nfq_data *nfa)
{
    static char newpayload[BUFFER_SIZE] __attribute__ ((aligned));
    int ret;
    struct iphdr* iph;
    struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr ( nfa );
    int id = ntohl(ph->packet_id);
    
    char *payload;
    u_int32_t payloadlen = nfq_get_payload(nfa, &payload);
    if (payloadlen>=0)
    {
        id = ntohl ( ph->packet_id );
        // retrieve IP header and information from datagram
        iph = (struct iphdr*) payload;

        DBG_HK(4,"LOCAL IN  : receive sk_buf (protocol# : %d)",  iph->protocol);
    
        //check to process only SEREADMO datagram
        if(iph->protocol != IPPROTO_MPOLSR)
        {
            DBG_HK(4,"LOCAL IN : do not process datagram");
            DBG_HK(4,"LOCAL IN : end - ACCEPT");
            return nfq_set_verdict ( qh, id, NF_ACCEPT, 0, NULL );
        }
    
        //char *newpayload;
        u_int32_t newpayloadlen=BUFFER_SIZE;
        ret = hk_process_datagram_local_in(iph, newpayload, newpayloadlen);
        if(ret < 0)
        {
            // failed to process
            DBG_HK(3,"LOCAL IN : end - DROP");
            return nfq_set_verdict ( qh, id, NF_DROP, 0, NULL );;
        }
        
        if(ret > 0)
        {
            // datagram forwarded
            DBG_HK(3,"LOCAL IN : datagram forwarded");
            DBG_HK(3,"LOCAL IN : end - DROP");
            return nfq_set_verdict(qh, id, NF_DROP, 0, NULL);;
        }

        if(ret == 0)
        {
            if (newpayloadlen>0)
            {
              DBG_HK(3,"LOCAL IN : datagram converted from SEREADMO to UDP");
              DBG_HK(3,"LOCAL IN : end - ACCEPT");

              // Accept modified packet
              return nfq_set_verdict ( qh, id, NF_ACCEPT, newpayloadlen, (unsigned char*)newpayload);
              
              // Free allocated buffer
              //delete [] newpayload;
              
              //return ret;
            }
            else
            {
              DBG_HK(3,"LOCAL IN : process ok");
              DBG_HK(3,"LOCAL IN : end - ACCEPT");
              return nfq_set_verdict ( qh, id, NF_ACCEPT, 0, NULL );
            }
              
        }
    }
    
    DBG_HK(3,"LOCAL IN : failed to get payload");
    DBG_HK(3,"LOCAL IN : end - DROP");
    return nfq_set_verdict ( qh, id, NF_DROP, 0, NULL );
}

}
}
