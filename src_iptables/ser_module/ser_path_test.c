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



void tc_print_path(struct olsr_path* path, int dbg_level)
{
    int                 index;
    ip_v4_addr          addr_ip;
    ip_v4_addr*         buff = path->path;

    if(g_dbg_hook< dbg_level)
        return;

    DBG_HK(dbg_level,"\n\n===================== PATH ======================\n");

    for(index = 0; index < path->path_hops; index ++)
    {
        memcpy(&addr_ip, buff++,ip_v4_size);
        DBG_HK(dbg_level,"Hop %d : %lX\n", index+1, addr_ip);
    }

    DBG_HK(dbg_level,"=================================================\n");
}



//***********************************************************************
//***********************************************************************


void tc_print_data(int withLock, int dbg_level)
{
    struct olsr_node*       node;
    struct olsr_node_link*  link;
    int                     index, index2;
    struct local_link*      local;

    if(g_dbg_hook< dbg_level)
        return;

    DBG_HK(dbg_level,"\n\n====================== TC =======================\n");

    if(withLock)
        READ_LOCK(g_ser_tc_lock);

    node = g_tc_node_lst;
    while(node)
    {
        DBG_HK(dbg_level,"-------------------------------------------\n"); 
        DBG_HK(dbg_level,"Node main addr : %lX\n", node->addr_tab[0]->addr_ip);

        link = node->link_src;
        while(link)
        {
            DBG_HK(dbg_level, "    to : %lX - cost : %03ld\n", link->dst_node->addr_tab[0]->addr_ip, link->cost);
            link = link->src_link_next;
        }

        link = node->link_dst;
        while(link)
        {
            DBG_HK(dbg_level,"  from : %lX - cost : %03ld\n", link->src_node->addr_tab[0]->addr_ip, link->cost) ;
            link = link->dst_link_next;
        }

        for(index2 = 1; index2 < MAX_IP_ALIAS_BY_NODE; index2++)
        {
            if(node->addr_tab[index2] != NULL)
            {
                DBG_HK(dbg_level,"Alias : %lX - %lX\n", node->addr_tab[0]->addr_ip, node->addr_tab[index2]->addr_ip);
            }
        }

        node = node->next_node;
    }

    DBG_HK(dbg_level,"-------------------------------------------\n"); 

    local = g_local_links;
    while(local)
    {

        DBG_HK(dbg_level,"Local link : dst: %lX - alias: %lX - local: %lX - mask: %lX\n", 
        local->dst_addr_ip,local->dst_alias_ip,local->local_addr_alias,local->local_addr_mask);

        local=local->next;
    }

    for(index = 0; index < MAX_IP_ALIAS_BY_NODE; index ++)
    {
        if(g_local_addr_tab[index] != 0)
            DBG_HK(dbg_level,"Local addr : %lX - %lX\n", g_local_addr_tab[index],g_local_mask_tab[index]);
    }

    if(withLock)
        READ_UNLOCK(g_ser_tc_lock);

    DBG_HK(dbg_level,"=================================================\n");
}


