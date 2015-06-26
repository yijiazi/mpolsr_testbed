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

#include "common.h"
#include "ser_path_def.h"
#include "ser_path.h"


static int pth_copy_path(struct olsr_path* path, char* path_data, unsigned long* pSize, ip_v4_addr* nextHop, int src_local, struct ser_header* pheader)
{
    struct local_link*          link;
    ip_v4_addr                  real_addr = 0;
    ip_v4_addr                  addr = 0;

    // add path size
    pheader->version  = MPOLSR_VERSION;
    pheader->hops     = htons(path->path_hops);
    pheader->cur      = htons(1);

    memcpy(path_data,pheader,sizeof(struct ser_header));

    // add path
    *pSize = path->path_hops * ip_v4_size;
    memcpy(path_data+sizeof(struct ser_header),path->path,*pSize);

    *pSize += sizeof(struct ser_header);

    //-------------------------------
    // retrieve real IP for first hop
    //-------------------------------
    DBG_HK(2,"find_datagram_path : retrieve IP for first hop in path");
    memcpy(&addr,path_data+sizeof(struct ser_header),ip_v4_size);


    if(src_local)
    {
        link = g_local_links;
        while(link)
        {
            if(link->dst_addr_ip == addr)
            {
                real_addr = link->dst_alias_ip;
                break;
            }

            link=link->next;
        }

        if(real_addr == 0)
            // TC data incomplete
            return -1;

        *nextHop = real_addr;
    }
    else
    {
        *nextHop = addr;
    }

    DBG_HK(2,"find_datagram_path : path and first hop (%s) ready for datagram update",ipToString(*nextHop));

    return 0;
}


//***********************************************************************
//***********************************************************************


int pth_find_datagram_path(char* path_data, unsigned long* pSize, ip_v4_addr srcAddr_req,
                        ip_v4_addr dstAddr_req, ip_v4_addr* nextHop, struct ser_header* pheader)
{
    struct olsr_path_buffer*    buffer;
    struct olsr_path*           path;
    int                         ret;
    ip_v4_addr                  dstAddr;
    ip_v4_addr                  srcAddr;
    struct olsr_node*           src_node;
    struct olsr_node*           dst_node;
    int                         src_local = 0;


    // retrieve main node IP (ie OLSRD IP) from requested src IP
    READ_LOCK(g_ser_tc_lock);
    src_node = tc_get_node(srcAddr_req);

    if(src_node == NULL)
    {
        DBG_HK(2,"find_datagram_path : failed to find src node (%s) in TC data",ipToString(srcAddr_req));
        READ_UNLOCK(g_ser_tc_lock);
        return -1;
    }
    srcAddr = src_node->addr_tab[0]->addr_ip;
    src_local = (srcAddr == g_local_addr_tab[0]);

    dst_node = tc_get_node(dstAddr_req);

    if(dst_node == NULL)
    {
        DBG_HK(2,"find_datagram_path : failed to find dst node (%s) in TC data",ipToString(dstAddr_req));
        READ_UNLOCK(g_ser_tc_lock);
        return -1;
    }
    dstAddr = dst_node->addr_tab[0]->addr_ip;


    DBG_HK(2,"find_datagram_path : search for path from %s to %s",ipToString(srcAddr),ipToString(dstAddr));

    //-------------------------
    // search for path in cache
    //-------------------------
    path = NULL;
    buffer = g_path_buffer;
    while(buffer)
    {
        if((buffer->src_addr_ip == srcAddr) && (buffer->dst_addr_ip == dstAddr))
        {

            DBG_HK(2,"find_datagram_path : path from %s to %s is in path cache",ipToString(srcAddr), ipToString(dstAddr));

            // find cache for src, dst nodes => get path list
            path = buffer->lst_path;

            // change current path to use all path and not only the first
            if(path != NULL)
                buffer->lst_path = path->next;

            break;
        }

        buffer = buffer->next;
    }

    if(path != NULL)
    {
        // path is in cache
        ret = pth_copy_path(path, path_data, pSize, nextHop,src_local,pheader);
        READ_UNLOCK(g_ser_tc_lock);
        return ret;
    }

    // path not in cache => need WRITE_LOCK to update TC data
    READ_UNLOCK(g_ser_tc_lock);

    if(buffer != NULL)
    {
        // already run tc_k_dijstra in a previous call, but no path found
        DBG_HK(2,"find_datagram_path : no path from %s to %s after k_dijstra", ipToString(srcAddr), ipToString(dstAddr));
        return -1;
    }


    //---------------------------------------
    // compute all paths if not in cache path
    //---------------------------------------

    WRITE_LOCK(g_ser_tc_lock);

    DBG_HK(2,"find_datagram_path : no path from %s to %s in path cache", ipToString(srcAddr), ipToString(dstAddr));
    // path for src to dst nodes not in path cache
    // => need find new path
    path = tc_k_dijstra(srcAddr,dstAddr);

    //-----------------------
    // process retrieved path
    //-----------------------
    if(path == NULL)
    {
        // oups !!! no available path from src to dst node with curren TC
        DBG_HK(2,"find_datagram_path : no path from %s to %s after k_dijstra", ipToString(srcAddr), ipToString(dstAddr));
        WRITE_UNLOCK(g_ser_tc_lock);
        return -1;
    }

    ret = pth_copy_path(path, path_data, pSize, nextHop,src_local,pheader);
    WRITE_UNLOCK(g_ser_tc_lock);
    return ret;
}


//***********************************************************************
//***********************************************************************


int pth_isEndOfPath(char* path)
{
    struct ser_header   header;
    int                 offset;
    ip_v4_addr          addr;
    int                 ret  =  -1;
    int                 cur;
    int                 hops;


    // check current hop index
    //------------------------
    memcpy(&header, path  ,sizeof(struct ser_header));
    cur = ntohs(header.cur);
    hops = ntohs(header.hops);


    if(cur > hops)
    {
        // current hop must be less than max
        MSG_FAILED("pth_isEndOfPath :  invalid path information (cur hop > #hops)");
        return -1;
    }

    if( hops != cur) // end of path only if current hop = total hops
    {
        DBG_HK(3,"pth_isEndOfPath : not end of path for datagram");
        return 0;
    }

    if(cur<1)       // must have at least 1 hop
    {
        // current hop must be less than max
        MSG_FAILED("pth_isEndOfPath :  invalid path information (cur < 1)");
        return -1;
    }

    // max and cur seem OK => check addr
    //-----------------------------------
    offset = (cur-1)*ip_v4_size + sizeof(struct ser_header);
    memcpy(&addr,path+offset,ip_v4_size);

    READ_LOCK(g_ser_tc_lock);
    if(addr ==  g_local_addr_tab[0])
    {
        DBG_HK(3,"pth_isEndOfPath : end of path for datagram");
        ret = 1;
    }
    else
    {
        MSG_FAILED("pth_isEndOfPath : current hop = max hops, but addr in path is not local node addr");
        ret = -1;
    }
    READ_UNLOCK(g_ser_tc_lock);

    return ret;  // hops == cur  => so addr must be equal

}

//***********************************************************************
//***********************************************************************

int pth_checkCurrentAddr(ip_v4_addr curr_addr)
{
    READ_LOCK(g_ser_tc_lock);
    
    if(curr_addr != g_local_addr_tab[0])
    {
        //ops! local node is not right node !!!
        MSG_FAILED("pth_checkCurrentAddr :  current hop is not for this node !!!");
        READ_UNLOCK(g_ser_tc_lock);
        return -1;
    }
    READ_UNLOCK(g_ser_tc_lock);
    return 0;
}

//***********************************************************************
//***********************************************************************

int pth_checkNextHop(ip_v4_addr* next_addr)
{
    struct local_link*      link;
    ip_v4_addr              real_addr;
    
    READ_LOCK(g_ser_tc_lock);
    
    // check if next hop exist in TC
    //------------------------------
    link = g_local_links;
    while(link)
    {
        if(link->dst_addr_ip == *next_addr)
        {
            real_addr = link->dst_alias_ip;
            break;
        }

        link=link->next;
    }

    READ_UNLOCK(g_ser_tc_lock);

    if(link == NULL)
    {
        // next hop unavailable
        DBG_HK(2,"pth_isEndOfPath : node for next hop not in neighbourhood => need path recovery");
        return 1;
    }

    if(real_addr == 0)
    {
        // TC data incomplete
        MSG_FAILED("pth_checkNextHop :  invalid TC data (path exist but local link info unavailable)");
        return -1;
    }

    *next_addr = real_addr;

    return 0;
    
}


//***********************************************************************
//***********************************************************************


int pth_printPath(char* path, unsigned long size, int dbg_level)
{
    struct ser_header       header;
    ip_v4_addr              addr;
    int                     index;
    char*                   buff = path;

    if(g_dbg_hook< dbg_level)
        return 0;

    DBG_HK(dbg_level,"\n\n===================== PATH ======================");

    memcpy(&header, path  ,sizeof(struct ser_header));
    buff += sizeof(struct ser_header);

    for(index= 0; index < ntohs(header.hops); index++)
    {
        memcpy(&addr,buff,ip_v4_size);
        buff += ip_v4_size;
        DBG_HK(dbg_level,"Hop %d : %s", index+1, ipToString(addr));
    }

    DBG_HK(dbg_level,"=================================================");

    return 0;

}


//***********************************************************************
//***********************************************************************


int pth_getLastNode(char* path, unsigned long size, ip_v4_addr* lastNode)
{
    struct ser_header       header;
    int                     offset;

    memcpy(&header, path, sizeof(struct ser_header));
    offset = sizeof(struct ser_header) + (ntohs(header.hops)-1) * ip_v4_size;
    memcpy(lastNode,path+offset,ip_v4_size);

    DBG_HK(3,"Last node in path : %s", ipToString(*lastNode));

    return 0;
}

