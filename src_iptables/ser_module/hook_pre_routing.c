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
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include "/usr/include/linux/netfilter_ipv4.h"
#include <net/ip.h>
#include "common.h"
#include "ser_path.h"
#include "skb_tools.h"

extern int ip_route_me_harder(struct sk_buff *skb, unsigned addr_type);

static int hk_route_recovery_pre_routing(struct sk_buff *skb, struct ser_header* pheader, char* initPath, 
unsigned long initSize)
{
    char                path[65536];
    unsigned long       size;
    struct sk_buff*     skb_new;
    struct iphdr*       iph;
    unsigned long       srcAddr;
    unsigned long       dstAddr;
    unsigned long       nextHop;
    int                 delta;


    // retreive last node addr
    if(pth_getLastNode(initPath,initSize,&dstAddr) <0)
    {
        MSG_FAILED("PRE ROUTING : Failed to retrieve last node in data path for route recovery\n");
        return -1;
    }

    iph     = ip_hdr(skb);
    srcAddr = iph->daddr;

    if(pth_find_datagram_path(path,&size, srcAddr, dstAddr,&nextHop,pheader) <0 )
    {
        // failed to find path
        MSG_FAILED("PRE ROUTING : failed to find path from %lX to %lX for route recovery\n",srcAddr, dstAddr);
        return -1;
    }

    if(size == 0)
        // datagram for localhost ??? => no path, no modification
        return -1;

    //----------------------------------
    //create new sk_buff with right size
    //----------------------------------
    delta = size-initSize;
    if(delta < 0)
        delta = 0;

    skb_new = skb_copy_expand(skb,0,delta,GFP_ATOMIC);
    if(skb_new == NULL)
    {
        MSG_FAILED("PRE ROUTING : Failed to expand sk_buff for route recovery\n");
        return -1;
    }
    if(delta>0)
        skb_put(skb_new,delta);


    //---------
    // add path
    //---------
    if(skb_replacePath(skb_new,path,size,initSize) < 0)
    {
        // failed to add path
        kfree_skb(skb_new);
        return -1;
    }

    delta = initSize-size;
    if(delta >0)
        // remove useless bytes
        skb_trim(skb,delta);

    //------------------------------------
    // update protocol number and next hop
    //------------------------------------
    iph = ip_hdr(skb_new);
    iph->protocol = 99;
    iph->daddr = nextHop;
    iph->saddr = srcAddr;

    //----------------------------------
    // update checksum and send datagram
    //----------------------------------
    skb_updateChecksumIP(skb_new);
    if(ip_route_me_harder(&skb_new, RTN_UNSPEC) != 0)
    {
        MSG_FAILED("PRE ROUTING : Failed to route new sk_buff for route recovery\n");
        return -1;
    }

    // send new datagram
    NF_HOOK(PF_INET,NF_IP_LOCAL_OUT,skb_new,NULL,skb_dst(skb_new)->dev,dst_output);

    return 1;

}


//***********************************************************************
//***********************************************************************


static int hk_process_datagram_pre_routing(struct sk_buff *skb)
{
    char path[65536];
    unsigned long size = 0;
    unsigned long nextHop;
    struct iphdr *iph;
    int ret;
    struct ser_header   header;

    //---------
    // get path
    //---------
    memset(&header,0, sizeof(struct ser_header));
    if (!skb_make_writable(&skb, skb->len))
    {
        MSG_FAILED("PRE ROUTING : Failed to make writable sk_buff\n");
        return -1;
    }

    iph = ip_hdr(skb);
    if(skb_getPath(skb, path, &size,&header) < 0)
        return -1;


    if(pth_isEndOfPath(path, size))
    {
        //datagram is for localhost

        // remove path from payload
        if(skb_removePath(skb,size) < 0)
            return -1;

        // restore protocol number and original node addr
        iph->protocol = header.protocol;
        iph->saddr = header.first_node;

        // update IP + UDP checksum
        skb_updateChecksumIP_UDP(skb);

        return 0;   // That's all folks !!!
    }

    //-----------------------------
    // update datagram for next hop
    //-----------------------------
    ret = pth_checkNextHop(path, size, &nextHop);
    if(ret < 0)
        // failed to get next hop
        return -1;

    if(ret > 0)
    {
        return hk_route_recovery_pre_routing(skb,&header,path,size);
    }

    // update datagram dst
    iph->daddr = nextHop;

    if(skb_updatePath(skb,path,size) <0)
        return -1;

    // update checksum
    skb_updateChecksumIP(skb);

    // datagram is auto-magicaly forwarded by the IP stack ...
    return 0;
}



//***********************************************************************
//***********************************************************************


unsigned int hk_pre_routing_hook(unsigned int hooknum,  struct sk_buff **skb, const struct net_device *in,
                   const struct net_device *out,  int (*okfn)(struct sk_buff *))
{
    int ret;

    // retrieve IP header and information
    struct iphdr *iph = ip_hdr(*skb);

    DBG_HK(4,"PRE ROUTING  : receive sk_buf (protocol# : %d)\n",  iph->protocol);

    // only process SEREADMO datagram
    if(iph->protocol != 99)
    {
        DBG_HK(4,"PRE ROUTING : do not process datagram\n");
        DBG_HK(4,"PRE ROUTING : end - ACCEPT\n");
        return NF_ACCEPT;
    }

    // check fragment flags:

    // in pre_routing hook we only take care of none fragmented datagram.
    // fragmented datagram are precessed in local_in hook
    if(iph->frag_off & htons(IP_MF))
    {
        // more fragments
        DBG_HK(3,"PRE ROUTING : fragmented datagram, do not process\n");
        DBG_HK(3,"PRE ROUTING : end - ACCEPT\n");
        return NF_ACCEPT;
    }

    if(iph->frag_off & htons(IP_OFFSET))
    {
        // last fragment
        DBG_HK(3,"PRE ROUTING : fragmented datagram (last frag), do not process\n");
        DBG_HK(3,"PRE ROUTING : end - ACCEPT\n");
        return NF_ACCEPT;
    }


    // this datagram is not fragmented we can process itf
    ret = hk_process_datagram_pre_routing(*skb);

    if(ret < 0)
    {
        // failed to process
        DBG_HK(3,"PRE ROUTING : end - DROP\n");
        return NF_DROP;
    }

    if(ret > 0)
    {
        // route recovery
        DBG_HK(3,"PRE ROUTING : process ok\n");
        DBG_HK(3,"PRE ROUTING : end - STOLEN\n");
        kfree_skb(*skb);
        return NF_STOLEN;
    }


    DBG_HK(3,"PRE ROUTING : process ok\n");
    DBG_HK(3,"PRE ROUTING : end - ACCEPT\n");
    return  NF_ACCEPT;
}

