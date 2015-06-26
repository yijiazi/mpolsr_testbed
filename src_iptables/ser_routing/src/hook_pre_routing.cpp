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
* pre routing hook
*
*************************************************************************
* Changes :
* --------
*
*************************************************************************/
#include <iostream>
#include <string.h>
#include <netinet/ip.h>

extern "C" {
#include "common.h"
#include <linux/netfilter_ipv4.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
}
#include "ser_path.h"

#include "mpolsrpacket.h"

//using namespace std;
using namespace sereadmo::net;

namespace sereadmo {
namespace nfhooks {

static int hk_route_recovery_pre_routing(struct iphdr* iph, char *newpayload, u_int32_t& newpayloadlen)
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
        MSG_FAILED("PRE ROUTING : Failed to retrieve last node in data path for route recovery");
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
        MSG_FAILED("PRE ROUTING : failed to find path from %lX to %lX for route recovery",srcAddr, dstAddr);
        return -1;
    }

    if(size == 0)
        // datagram for localhost ??? => no path, no modification
        return -1;

    if (MpOlsrPacket::addMpOlsrHeader(iph, path, size, newpayload, newpayloadlen)==false)
    {
        MSG_FAILED("PRE ROUTING : Failed to add MpOlsr header");
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


//***********************************************************************
//***********************************************************************


static int hk_process_datagram_pre_routing(struct iphdr* iph, char *newpayload, u_int32_t& newpayloadlen)
{
    char* path=NULL;
    unsigned long size = 0;
    ip_v4_addr curAddr;
    ip_v4_addr nextAddr;
    ip_v4_addr srcAddr;
    int ret;
    
    //newpayload = NULL;
    //newpayloadlen=0;

    // retrieve path
    MpOlsrPacket packet(iph); 
    
    if (packet.getMpOlsrHeader()==NULL)
    {
        MSG_FAILED("PRE ROUTING : canot get mpolsr header");
        return -1;
    }

    curAddr = packet.getCurrentAddr();
    if (curAddr<=0)
    {
        MSG_FAILED("PRE ROUTING : canot get current address in mpolsr header");
        return -1;
    }
        
    if (pth_checkCurrentAddr(curAddr))
    {
        MSG_FAILED("PRE ROUTING : pth_checkCurrentAddr returned false");
        return -1;
    }
    
    //------------------------------------
    // check if local host is datagram dst
    //------------------------------------
    if(packet.isEndOfPath())
    {
        DBG_HK(4,"PRE ROUTING : isEndOfPath");
        // datagram is for localhost
        // remove path from payload
        if (packet.removeMpOlsrHeader(newpayload, newpayloadlen)==false)
            return -1;

        newpayloadlen = packet.getIpTotalSize() - packet.getMpOlsrHeaderSize();

        return 0;   // That's all folks !!!
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
    {
        return hk_route_recovery_pre_routing(iph, newpayload, newpayloadlen);
    }

    // update datagram dst
    iph->daddr = nextAddr;
    iph->saddr = srcAddr;

    if (packet.moveToNextHop()==false)
        // failed to update path information
        return -1;

    DBG_HK(4,"PRE ROUTING : forwarding to %s",  ipToString(packet.getCurrentAddr()));
    //----------------------------------
    // update checksum 
    //----------------------------------
    packet.updateIpChecksum(iph);

    newpayloadlen=newpayloadlen = packet.getIpTotalSize();
    memcpy(newpayload, iph, newpayloadlen);
    
    // datagram is auto-magicaly forwarded by the IP stack ...
    return 0;
}



//***********************************************************************
//***********************************************************************


unsigned int hk_pre_routing_hook(struct nfq_q_handle *qh, struct nfq_data *nfa)
{
    static char newpayload[BUFFER_SIZE] __attribute__ ((aligned));
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
        DBG_HK(4,"PRE ROUTING : receive sk_buf (protocol# : %d)",  iph->protocol);
        
        // only process SEREADMO datagram
        if(iph->protocol != IPPROTO_MPOLSR)
        {
            DBG_HK(3,"PRE ROUTING : do not process datagram");
            DBG_HK(3,"PRE ROUTING : end - ACCEPT");
            return nfq_set_verdict ( qh, id, NF_ACCEPT, 0, NULL );
        }

        // check fragment flags:
        
        // in pre_routing hook we only take care of non fragmented datagram.
        // fragmented datagram are precessed in local_in hook
        if(iph->frag_off & htons(IP_MF))
        {
            // more fragments
            DBG_HK(3,"PRE ROUTING : fragmented datagram, do not process");
            DBG_HK(3,"PRE ROUTING : end - ACCEPT");
            return nfq_set_verdict( qh, id, NF_ACCEPT, 0, NULL );
        }

    if(iph->frag_off & htons(IP_OFFMASK))
    {
        // last fragment
        DBG_HK(3,"PRE ROUTING : fragmented datagram (last frag), do not process");
        DBG_HK(3,"PRE ROUTING : end - ACCEPT");
        return nfq_set_verdict ( qh, id, NF_ACCEPT, 0, NULL );
    }


    // this datagram is not fragmented we can process it
    u_int32_t newpayloadlen=BUFFER_SIZE;
    int ret = hk_process_datagram_pre_routing(iph, newpayload, newpayloadlen);

    if(ret < 0)
    {
        // failed to process
        DBG_HK(3,"PRE ROUTING : end - DROP");
        return nfq_set_verdict ( qh, id, NF_DROP, 0, NULL );;
    }

    if(ret > 0)
    {
        // route recovery
        DBG_HK(3,"PRE ROUTING : route recovery");
        DBG_HK(3,"PRE ROUTING : end - ACCEPT");
        // Accept modified packet
        return nfq_set_verdict ( qh, id, NF_ACCEPT, newpayloadlen, (unsigned char*)newpayload);
        
        // Free allocated buffer
        //delete [] newpayload;
        
        //return ret;
    }
    
    if(ret == 0)
    {
        if (newpayloadlen>0)
        {
          DBG_HK(3,"PRE ROUTING : datagram converted from SEREADMO to UDP");
          DBG_HK(3,"PRE ROUTING : end - ACCEPT");

          // Accept modified packet
          return nfq_set_verdict ( qh, id, NF_ACCEPT, newpayloadlen, (unsigned char*)newpayload);
          
          // Free allocated buffer
          //delete [] newpayload;
          
          //return ret;
        }
        else
        {
          DBG_HK(3,"PRE ROUTING : datagram forwarded");
          DBG_HK(3,"PRE ROUTING : end - ACCEPT");
          
          return nfq_set_verdict ( qh, id, NF_ACCEPT, payloadlen, (unsigned char*)payload );
        }
          
    }
    

  }
    DBG_HK(3,"LOCAL IN : failed to get payload");
    DBG_HK(3,"LOCAL IN : end - DROP");
    return nfq_set_verdict ( qh, id, NF_DROP, 0, NULL );
}

}
}
