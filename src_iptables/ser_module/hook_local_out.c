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
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include "/usr/include/linux/netfilter_ipv4.h"
#include "common.h"
#include "ser_path.h"
#include "skb_tools.h"

extern int ip_route_me_harder(struct sk_buff *skb, unsigned addr_type);

//***********************************************************************
//***********************************************************************

// check if sk_buff need to be update (ie add multi-path, ...)
int hk_check_if_update(struct sk_buff **skb, const struct net_device *out)
{
    struct iphdr  *iph;
    struct udphdr *uh;

    // retrieve IP header
    iph = ip_hdr(*skb);

    // check protocol number (only UDP datagram are updated)
    if(iph->protocol != IPPROTO_UDP)
        return 0;

    // check dst => if dst is local host => no modification
    if(out && (memcmp(out->name,"lo",2) == 0))
        return 0;

    // check port number (some UDP port are not processed)
    uh = (struct udphdr*)((*skb)->data+(iph->ihl<<2));
    DBG_HK(4,"LOCAL OUT : UDP port : %d)\n",  ntohs(uh->dest));
    if(ntohs(uh->dest) != g_conf_udp_port)
        return 0;

    return 1;
}



//***********************************************************************
//***********************************************************************


int hk_process_datagram_local_out(struct sk_buff *skb)
{
    char                path[65536];
    unsigned long       size;
    struct sk_buff*     skb_new;

    struct iphdr*       iph;
    unsigned long       srcAddr;
    unsigned long       dstAddr;
    unsigned long       nextHop;
    struct ser_header   header;

    //------------------------------
    // retrieve datagram information
    //------------------------------
    // retrieve IP header
    iph = ip_hdr(skb);
    srcAddr = iph->saddr;
    dstAddr = iph->daddr;

    //-------------------
    // find datagram path
    //-------------------
    size = 65536;
    memset(&header,0, sizeof(struct ser_header));
    header.protocol     = iph->protocol;
    header.first_node   = srcAddr;

    if(pth_find_datagram_path(path,&size, srcAddr, dstAddr,&nextHop,&header) <0 )
    {
        // failed to find path
        DBG_HK(4,"LOCAL OUT : failed to find path from %lX to %lX\n",srcAddr, dstAddr);
        return -1;
    }

    if((size == sizeof(struct ser_header)) || (size ==0))
        // datagram for localhost ??? => no path, no modification
        return 0;


    //----------------------------------
    //create new sk_buff with right size
    //----------------------------------
    skb_new = skb_copy_expand(skb,0,size,GFP_ATOMIC);
    if(skb_new == NULL)
    {
        MSG_FAILED("LOCAL OUT : Failed to expand sk_buff\n");
        return -1;
    }
    skb_put(skb_new,size);


    //---------
    // add path
    //---------
    if(skb_addPath(skb_new,path,size) < 0)
    {
        // failed to add path
        kfree_skb(skb_new);
        return -1;
    }


    //------------------------------------
    // update protocol number and next hop
    //------------------------------------
    iph = ip_hdr(skb_new);
    iph->protocol = 99;
    iph->daddr = nextHop;
    //iph->saddr = srcAddr;

    //----------------------------------
    // update checksum and send datagram
    //----------------------------------
    skb_updateChecksumIP(skb_new);
    if(ip_route_me_harder(&skb_new, RTN_UNSPEC) != 0)
    {
        MSG_FAILED("LOCAL OUT : Failed to route new sk_buff\n");
        return -1;
    }

    // send new datagram
    NF_HOOK(PF_INET,NF_IP_LOCAL_OUT,skb_new,NULL,skb_dst(skb_new)->dev,dst_output);

    return 1;

}


//***********************************************************************
//***********************************************************************


unsigned int hk_local_out_hook(unsigned int hooknum,  struct sk_buff **skb, const struct net_device *in,
                   const struct net_device *out,  int (*okfn)(struct sk_buff *))
{
    int ret;
    struct iphdr    *iph;

    iph = ip_hdr(*skb);
    DBG_HK(4,"LOCAL OUT : receive sk_buf (protocol# : %d)\n",  iph->protocol);

    if(!hk_check_if_update(skb, out))
    {
        // we do not modify this datagram
        DBG_HK(4,"LOCAL OUT : do not process datagram\n");
        DBG_HK(4,"LOCAL OUT : end - ACCEPT\n");
        return NF_ACCEPT;
    }

    DBG_HK(3,"LOCAL OUT : process datagram\n");
    ret = hk_process_datagram_local_out(*skb);
    if(ret < 0)
    {
        // failed to update datagram
        DBG_HK(3,"LOCAL OUT : end - DROP\n");
        return NF_DROP;
    }

    if(ret == 0)
    {
        // no datagram modification
        DBG_HK(3,"LOCAL OUT : end - ACCEPT\n");
        return NF_ACCEPT;
    }

    DBG_HK(3,"LOCAL OUT : process ok\n");
    DBG_HK(3,"LOCAL OUT : end - STOLEN\n");
    kfree_skb(*skb);
    return NF_STOLEN;
}

