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


int hk_process_datagram_local_in(struct sk_buff *skb)
{
    char path[65536];
    unsigned long size = 0;
    unsigned long nextHop;
    unsigned long srcAddr;
    struct iphdr *iph;
    struct sk_buff  *skb_new;
    int ret;
    struct ser_header   header;


    // in 'local in' hook we process fragmented datagram
    // => it's to late to forward this datagram if local host,
    //    is not dst. => we must send an other with same data

    // to simplify data manipulation we must linearize sk_buff
    // (because of fragments)
    if(skb_linearize(skb) < 0)
    {
        MSG_FAILED("LOCAL IN : Failed to linearize sk_buff");
        return -1;
    }

    // retrieve path
    memset(&header,0, sizeof(struct ser_header));
    if(skb_getPath(skb, path, &size,&header) < 0)
        return -1;

    //------------------------------------
    // check if local host is datagram dst
    //------------------------------------
    if(pth_isEndOfPath(path, size))
    {
        //datagram is for localhost

        if (!skb_make_writable(&skb, skb->len))
        {
            MSG_FAILED("LOCAL IN : Failed to make writable sk_buff");
            return -1;
        }

        iph = ip_hdr(skb);

        // remove path from payload
        if(skb_removePath(skb,size) < 0)
            return -1;

        // restore protocol number
        iph->protocol = header.protocol;
        iph->saddr = header.first_node;

        // update IP + UDP checksum
        skb_updateChecksumIP_UDP(skb);

        return 0;   // That's all folks !!!
    }

    // local host is not the right dst => send new sk_buff
    iph = ip_hdr(skb);
    srcAddr = iph->daddr;

    //------------------
    //create new sk_buff
    //------------------
    skb_new = skb_copy(skb,GFP_ATOMIC);
    if(skb_new == NULL)
    {
        MSG_FAILED("LOCAL IN : Failed to copy sk_buff\n");
        return -1;
    }
    iph = ip_hdr(skb_new);

    //-----------------------------
    // update datagram for next hop
    //-----------------------------
    ret = pth_checkNextHop(path, size, &nextHop);
    if(ret < 0)
        // failed to get next hop
        return -1;

    if(ret > 0)
        // next hop not available => need route recovery
        return -1; // TODO

    // update datagram dst
    iph->daddr = nextHop;
    iph->saddr = srcAddr;

    if(skb_updatePath(skb_new,path,size) <0)
        // failed to update path information
        return -1;


    //----------------------------------
    // update checksum and send datagram
    //----------------------------------
    skb_updateChecksumIP(skb_new);
    if(ip_route_me_harder(&skb_new, RTN_UNSPEC) != 0)
    {
        MSG_FAILED("LOCAL IN : Failed to route new sk_buff\n");
        return -1;
    }

    // send new datagram
    NF_HOOK(PF_INET,NF_IP_LOCAL_OUT,skb_new,NULL,skb_dst(skb_new)->dev,dst_output);

    return  1;
}



//***********************************************************************
//***********************************************************************


unsigned int hk_local_in_hook(unsigned int hooknum,  struct sk_buff **skb, const struct net_device *in,
                   const struct net_device *out,  int (*okfn)(struct sk_buff *))
{
    int ret;

    // retrieve IP header and information from datagram
    struct iphdr *iph = ip_hdr(*skb);
    DBG_HK(4,"LOCAL IN  : receive sk_buf (protocol# : %d)\n",  iph->protocol);

    //check to process only SEREADMO datagram
    if(iph->protocol != 99)
    {
        DBG_HK(4,"LOCAL IN : do not process datagram\n");
        DBG_HK(4,"LOCAL IN : end - ACCEPT\n");
        return NF_ACCEPT;
    }

    ret = hk_process_datagram_local_in(*skb);
    if(ret < 0)
    {
        // failed to process
        DBG_HK(3,"LOCAL IN : end - DROP\n");
        return NF_DROP;
    }

    if(ret == 0)
    {
        DBG_HK(3,"LOCAL IN : datagram converted from SEREADMO to UDP\n");
        DBG_HK(3,"LOCAL IN : end - ACCEPT\n");
        return NF_ACCEPT;
    }

    // route recovery
    DBG_HK(3,"LOCAL IN : process ok\n");
    DBG_HK(3,"LOCAL IN : end - STOLEN\n");
    kfree_skb(*skb);
    return NF_STOLEN;

}
