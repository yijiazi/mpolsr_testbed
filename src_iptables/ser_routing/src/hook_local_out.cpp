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
* local out hook
*
*************************************************************************
* Changes :
* --------
*
*************************************************************************/
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

extern "C" {
#include "common.h"
#include <linux/netfilter_ipv4.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
}
#include "ser_path.h"
//#include "skb_tools.h"

#include "mpolsrpacket.h"

using namespace sereadmo::net;

namespace sereadmo {
namespace nfhooks {

//***********************************************************************
//***********************************************************************

// check if sk_buff need to be update (ie add multi-path, ...)
int hk_check_if_update(struct iphdr* iph)
{
    struct udphdr *uh;

    // check protocol number (only UDP datagram are updated)
    if(iph->protocol != IPPROTO_UDP)
        return 0;

    // check dst => if dst is local host => no modification
    if (iph->daddr == INADDR_LOOPBACK)
    //if(out && (memcmp(out->name,"lo",2) == 0))
        return 0;

    // check port number (some UDP port are not processed)
    uh = (struct udphdr*)((char*)iph + (iph->ihl<<2));
    DBG_HK(4,"LOCAL OUT : UDP port : %d)",  ntohs(uh->dest));
    if(ntohs(uh->dest) != g_conf_udp_port)
        return 0;

    return 1;
}



//***********************************************************************
//***********************************************************************

int hk_process_datagram_local_out(struct iphdr* iph, char *newpayload, u_int32_t& newpayloadlen)
{
    static char path[MAX_MPOLSR_PATH] __attribute__ ((aligned));
    
    unsigned long       size;
    struct sk_buff*     skb_new;

    unsigned long       srcAddr;
    unsigned long       dstAddr;
    unsigned long       nextHop;
    struct ser_header   header;

    //------------------------------
    // retrieve datagram information
    //------------------------------
    // retrieve IP header
    srcAddr = iph->saddr;
    dstAddr = iph->daddr;

    //-------------------
    // find datagram path
    //-------------------
    size = MAX_MPOLSR_PATH;
    memset(&header,0, sizeof(struct ser_header));
    header.protocol     = iph->protocol;
    header.first_node   = srcAddr;

    if(pth_find_datagram_path(path,&size, srcAddr, dstAddr,&nextHop,&header) <0 )
    {
        // failed to find path
        DBG_HK(4,"LOCAL OUT : failed to find path from %s to %s",ipToString(srcAddr),ipToString(dstAddr));
        return -1;
    }

    if((size == sizeof(struct ser_header)) || (size ==0))
        // datagram for localhost ??? => no path, no modification
        return 0;

    if (MpOlsrPacket::addMpOlsrHeader(iph, path, size, newpayload, newpayloadlen)==false)
    {
        MSG_FAILED("LOCAL OUT : Failed to add MpOlsr header");
        return -1;
    }
    struct iphdr* iphNew = (struct iphdr*)newpayload;
    IpPacket ip(iphNew);
    newpayloadlen=ip.getIpTotalSize();

    //------------------------------------
    // update protocol number and next hop
    //------------------------------------
    iphNew->protocol = IPPROTO_MPOLSR;
    iphNew->daddr = nextHop;

    //----------------------------------
    // update checksum and accept datagram
    //----------------------------------
    IpPacket::updateIpChecksum(iphNew);

    return 1;
}


//***********************************************************************
//***********************************************************************


unsigned int hk_local_out_hook(struct nfq_q_handle *qh, struct nfq_data *nfa)
{
    static char newpayload[BUFFER_SIZE] __attribute__ ((aligned));
    struct iphdr* iph;
    int ret;
    struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr ( nfa );
    int id = ntohl(ph->packet_id);
    
    char *payload;
    u_int32_t payloadlen = nfq_get_payload(nfa, &payload);
    if (payloadlen>=0)
    {
        id = ntohl ( ph->packet_id );
        // retrieve IP header and information from datagram
        iph = (struct iphdr*) payload;

        DBG_HK(4,"LOCAL OUT : receive packet (protocol# : %d)",  iph->protocol);

        if(!hk_check_if_update(iph))
        {
            // we do not modify this datagram
            DBG_HK(4,"LOCAL OUT : do not process datagram");
            DBG_HK(4,"LOCAL OUT : end - ACCEPT");
            return nfq_set_verdict ( qh, id, NF_ACCEPT, 0, NULL );
        }

        DBG_HK(3,"LOCAL OUT : process datagram");
        u_int32_t newpayloadlen=BUFFER_SIZE;
        ret = hk_process_datagram_local_out(iph, newpayload, newpayloadlen);
        if(ret < 0)
        {
            // failed to update datagram
            DBG_HK(3,"LOCAL OUT : end - DROP");
            return nfq_set_verdict ( qh, id, NF_DROP, 0, NULL );
        }
        
        if(ret == 0)
        {
            // no datagram modification
            DBG_HK(3,"LOCAL OUT : end - ACCEPT");
            return nfq_set_verdict ( qh, id, NF_ACCEPT, 0, NULL );
        }

        DBG_HK(3,"LOCAL OUT : converted from UDP to SEREADMO");
        DBG_HK(3,"LOCAL OUT : end - ACCEPT");
        // Accept modified datagram
        ret = nfq_set_verdict ( qh, id, NF_ACCEPT, newpayloadlen, (unsigned char*)newpayload);
        // Free allocated buffer
        //delete [] newpayload;
        return ret;
    }
    
    // Coulnd't get payload, let datagram go
    DBG_HK(4,"LOCAL OUT : couldn't get payload");
    DBG_HK(4,"LOCAL OUT : end - ACCEPT");
    return nfq_set_verdict ( qh, id, NF_ACCEPT, 0, NULL );
}

}
}
