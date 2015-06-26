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

static int djk_printPath(struct olsr_path* path, ip_v4_addr srcAddr)
{
    int index;
    DBG_DJK(3,"Src   : %s", ipToString(srcAddr));
    for(index= 0; index < path->path_hops; index++)
    {
        DBG_DJK(3,"Hop %d : %s", index+1, ipToString(path->path[index]));
    }

    return 0;
}

static int dijkstra(ip_v4_addr srcAddr)
{
    struct olsr_node*       node_list;
    struct olsr_node*       node;
    struct olsr_node*       node_prev;
    struct olsr_node*       node_min_cost;
    struct olsr_node*       node_prev_min_cost;
    struct olsr_node_link*  link;
    unsigned long           min_cost;
    int                     count;

    DBG_DJK(1,"--- Start Dijkstra ---");

    DBG_DJK(2,"Dijkstra : init list node");
    //----------------------------------------
    // init node list to process and node cost
    //----------------------------------------
    node_list = NULL;
    count = 0;

    node = g_tc_node_lst;
    while(node)
    {
        node->dijkstra_list = node_list;
        node->dijkstra_processed = 0;
        node->dijkstra_path = NULL;
        node_list = node;

        if(node->addr_tab[0]->addr_ip == srcAddr)
            node->dijkstra_cost = 0;
        else
            node->dijkstra_cost = -1;

        node = node->next_node;
        count++;
    }



    // main dijkstra loop
    while(node_list != NULL)
    {
        DBG_DJK(4,"Dijkstra : search min cost node (%d nodes to process)", count);
        //--------------------------
        // find node with least cost
        //--------------------------
        min_cost = -1;
        node_prev = NULL;
        node_min_cost = NULL;
        node_prev_min_cost = NULL;
        count=0;

        node = node_list;
        while(node)
        {
            if(node->dijkstra_cost < min_cost)
            {
                node_prev_min_cost = node_prev;
                node_min_cost = node;
                min_cost = node->dijkstra_cost;
            }

            node_prev = node;
            node = node->dijkstra_list;
            count++;
        }


        if(node_min_cost == NULL)
        {
            // no more node to reach from src node even if there is more nodes in the network ....
            return 0;
        }

        DBG_DJK(4,"Dijkstra : update node : %s", ipToString(node_min_cost->addr_tab[0]->addr_ip));

        //-------------------------------------
        // remove min node from list to process
        //-------------------------------------
        if(node_prev_min_cost == NULL)
            // first node
            node_list = node_min_cost->dijkstra_list;
        else
            // not first one
            node_prev_min_cost->dijkstra_list = node_min_cost->dijkstra_list;

        node_min_cost->dijkstra_processed = 1;

        //------------------------------
        // update min node neighbourhood
        //------------------------------
        link = node_min_cost->link_src;
        while(link)
        {
            if(link->dst_node->dijkstra_processed == 0)
            {
                min_cost  = node_min_cost->dijkstra_cost + link->dijkstra_cost;
                if(min_cost < link->dst_node->dijkstra_cost)
                {
                    link->dst_node->dijkstra_cost = min_cost;
                    link->dst_node->dijkstra_path = node_min_cost;
                    link->dst_node->dijkstra_hops = node_min_cost->dijkstra_hops + 1;
                }
            }

            link = link->src_link_next;
        }
    }

    DBG_DJK(1,"--- Stop Dijkstra ---");

    return 0;
}

//***********************************************************************
//***********************************************************************

struct olsr_path* tc_k_dijstra(ip_v4_addr srcAddr, ip_v4_addr dstAddr)
{
    struct olsr_node*           node;
    struct olsr_node*           prev_node;
    struct olsr_node*           dst_node=NULL;
    struct olsr_node_link*      link;
    struct olsr_path*           first = NULL;
    struct olsr_path*           path = NULL;
    struct olsr_path_buffer*    path_buffer;
    int                         index_path;
    ip_v4_addr*                 buff;

    //-------------------------------------
    // add cache entry for src to dst nodes
    //-------------------------------------
    path_buffer = ALLOC_PATH_BUFFER;
    if(path_buffer == NULL)
    {
        MSG_FAILED("Failed to allocate memory for new path buffer" );
        return NULL;
    }
    memset(path_buffer,0,sizeof(struct olsr_path_buffer));

    path_buffer->src_addr_ip = srcAddr;
    path_buffer->dst_addr_ip = dstAddr;

    path_buffer->next = g_path_buffer;
    g_path_buffer = path_buffer;


    //-------------------------------------------------------------------
    // reset link dijstra cost on all network links and retrieve dst node
    //-------------------------------------------------------------------
    dst_node = tc_get_node(dstAddr);

    if(dst_node == NULL)
    {
        MSG_FAILED("Failed to find dst node in dijkstra algorithm");
        return NULL;
    }


    // initialise k_dijstra cost information with real link cost
    node = g_tc_node_lst;
    while(node)
    {
        link = node->link_src;
        while(link)
        {
            link->dijkstra_cost     = link->cost;
            link->dijkstra_cost_p1  = link->cost;
            link = link->src_link_next;
        }

        node = node->next_node;
    }

    //-----------------
    // compute all path
    //-----------------
    for(index_path = 0; index_path < g_conf_path; index_path++)
    {
        // find path to all nodes from src
        dijkstra(srcAddr);

        // retrieve path from src to dst nodes
        if(dst_node->dijkstra_cost == -1)
            //No path to dst node
            return NULL;

        path = NULL;
        path = ALLOC_PATH;
        if(path == NULL)
        {
            MSG_FAILED( "Failed to allocate memory for new path" );
            return NULL;
        }
        memset(path, 0, sizeof(struct olsr_path));

        path->path_hops = dst_node->dijkstra_hops;
        path->path = ALLOC_PATH_DATA(path->path_hops*ip_v4_size);
        if(path->path == NULL)
        {
            MSG_FAILED("Failed to allocate memory for new path" );
            FREE_PATH(path);
            return NULL;
        }
        memset(path->path ,0, path->path_hops*ip_v4_size);

        buff = path->path;
        buff += (dst_node->dijkstra_hops-1);

        node = dst_node;
        prev_node = NULL;
        while(node)
        {
            // copy ip for netfilter
            if(node->dijkstra_path !=NULL) // do not copy src IP
                memcpy(buff--,&node->addr_tab[0]->addr_ip,ip_v4_size); // use main addr (ie olsrd addr) in path

            // update link cost
            if(prev_node != NULL)
            {
                //retrieve link from node to prev_node (ie src -> dst)
                link = prev_node->link_dst;
                while(link)
                {
                    if(link->src_node == node)
                    {
                        link->dijkstra_cost_p1 = FP(link->dijkstra_cost);
                    }
                    else
                    {
                        link->dijkstra_cost_p1 = FE(link->dijkstra_cost);
                    }

                    link = link->dst_link_next;
                }

                // retrieve link from prev_node to node (ie dst -> src)
                link = prev_node->link_src;
                while(link)
                {
                    if(link->dst_node == node)
                        break;

                    link = link->src_link_next;
                }

                if(link != NULL)
                    link->dijkstra_cost_p1 = FP(link->dijkstra_cost);

            }
            prev_node = node;
            node = node->dijkstra_path;
        }

        node = g_tc_node_lst;
        while(node)
        {
            link = node->link_src;
            while(link)
            {
                link->dijkstra_cost = link->dijkstra_cost_p1;
                link = link->src_link_next;
            }
            node = node->next_node;
        }
        
        if(g_dbg_djk>=3)
          djk_printPath(path, srcAddr);
        
        // update cache entry
        if(path_buffer->lst_path == NULL)
        {
            // first path
            path->next = path;
            path_buffer->lst_path = path;
            first = path;
        }
        else
        {
            path->next = path_buffer->lst_path->next;
            path_buffer->lst_path->next = path;
        }

    }

    return first;
}


