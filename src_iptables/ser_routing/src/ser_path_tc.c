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
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

struct olsr_node_addr*      g_tc_addr_tab[HASH_TAB_SIZE];
struct olsr_node*           g_tc_node_lst = NULL;
struct olsr_path_buffer*    g_path_buffer = NULL;

struct kmem_cache*          g_ser_tc_node_add_cache = NULL;
struct kmem_cache*          g_ser_tc_node_cache = NULL;
struct kmem_cache*          g_ser_tc_link_cache = NULL;
struct kmem_cache*          g_ser_tc_path_cache = NULL;
struct kmem_cache*          g_ser_tc_path_buffer_cache = NULL;
struct kmem_cache*          g_ser_local_link_cache = NULL;

ip_v4_addr                  g_local_addr_tab[MAX_IP_ALIAS_BY_NODE];
ip_v4_addr                  g_local_mask_tab[MAX_IP_ALIAS_BY_NODE];


struct local_link*          g_local_links = NULL;

#ifndef APP_TEST
    rwlock_t                g_ser_tc_lock = RW_LOCK_UNLOCKED;
#else
    pthread_mutex_t         g_ser_tc_lock = PTHREAD_MUTEX_INITIALIZER;
#endif


//***********************************************************************
//***********************************************************************

// get hash key from IP address
static unsigned char get_hash_key(ip_v4_addr addr_ip)
{
    char* buff_ip = (char*) &addr_ip;
    return (unsigned char) *buff_ip;
}

//***********************************************************************
//***********************************************************************


static int tc_add_node_addr(struct olsr_node_addr* node_addr, struct olsr_node* node, ip_v4_addr addr_ip, int main_addr)
{
    struct olsr_node_addr* node_addr_tmp;
    int index, pos;
    int hash_key = get_hash_key(addr_ip);

    node_addr->node = node;
    node_addr->addr_ip = addr_ip;

    pos = -1;
    if(main_addr != 0)
    {
        // node main addr (ie addr used by OLSRD)
        pos = 0;
    }
    else
    {
        for(index = 1; index < MAX_IP_ALIAS_BY_NODE; index++)
        {
            if(node->addr_tab[index] == NULL)
            {
                pos = index;
                break;
            }
        }
    }

    if(pos < 0)
    {
        MSG_FAILED("Failed to add new node addr");
        return -1;
    }

    node_addr->addr_pos = pos;
    node->addr_tab[pos] = node_addr;

    node_addr_tmp = g_tc_addr_tab[hash_key];
    if(node_addr_tmp != NULL)
    {
        node_addr_tmp->hash_prev = node_addr;
        node_addr->hash_next = node_addr_tmp;
    }

    g_tc_addr_tab[hash_key] = node_addr;

    return 0;
}


//***********************************************************************
//***********************************************************************


static struct olsr_node_addr* tc_rmv_node_addr(ip_v4_addr addr_ip)
{
    struct olsr_node_addr* node_addr = NULL;
    int hash_key = get_hash_key(addr_ip);

    node_addr = g_tc_addr_tab[hash_key];
    while(node_addr)
    {
        if(node_addr->addr_ip == addr_ip)
        {
            if(node_addr->hash_prev == NULL)
                // first node with this hash key
                g_tc_addr_tab[hash_key] = node_addr->hash_next;
            else
                node_addr->hash_prev->hash_next = node_addr->hash_next;

            if(node_addr->hash_next != NULL)
                node_addr->hash_next->hash_prev = node_addr->hash_prev;

            break;
        }

        node_addr = node_addr->hash_next;
    }

    if(node_addr == NULL)
    {
        MSG_FAILED("Failed to remove node addr");
        return NULL;
    }

    node_addr->node->addr_tab[node_addr->addr_pos] = NULL;
    return node_addr;
}


//***********************************************************************
//***********************************************************************


struct olsr_node* tc_get_node(ip_v4_addr addr_ip)
{
    struct olsr_node_addr* node_addr = NULL;
    int hash_key = get_hash_key(addr_ip);

    node_addr = g_tc_addr_tab[hash_key];
    while(node_addr)
    {
        if(node_addr->addr_ip == addr_ip)
        {
            return node_addr->node;
        }

        node_addr = node_addr->hash_next;
    }

    return NULL;
}


//***********************************************************************
//***********************************************************************

static void free_path_buffer(struct olsr_path_buffer* buff)
{
    struct olsr_path_buffer* curr = buff;
    struct olsr_path_buffer* curr_tmp;
    struct olsr_path* path;
    struct olsr_path* first;
    struct olsr_path* path_tmp;

    if(buff == NULL)
        return;

    while(curr)
    {
        path = curr->lst_path;
        first = path;
        while(path)
        {
            if(path->next == first)
                path->next = NULL;

            path = path->next;
        }

        path = curr->lst_path;
        while(path)
        {
            path_tmp = path;
            path = path->next;
            FREE_PATH_DATA(path_tmp->path);
            FREE_PATH(path_tmp);
        }
        curr_tmp = curr;
        curr = curr->next;
        FREE_PATH_BUFFER(curr_tmp);
    }

}


//***********************************************************************
//***********************************************************************


static void tc_update_local_link(struct olsr_node* node_dst, struct local_link* local_link)
{
    int index1, index2;
    ip_v4_addr tmp, tmp2;

    local_link->dst_addr_ip = node_dst->addr_tab[0]->addr_ip; // main node addr
    local_link->dst_alias_ip = 0;
    local_link->local_addr_alias = 0;
    local_link->local_addr_mask = 0;


    for(index1=0; index1 < MAX_IP_ALIAS_BY_NODE; index1++)
    {
        tmp = g_local_addr_tab[index1] & g_local_mask_tab[index1];

        if(tmp == 0)
            continue;

        for(index2=0; index2 < MAX_IP_ALIAS_BY_NODE; index2++)
        {
            if(node_dst->addr_tab[index2] == NULL)
                continue;

            tmp2 = node_dst->addr_tab[index2]->addr_ip & g_local_mask_tab[index1];

            if(tmp2 == tmp)
            {
                // find right network interface
                local_link->local_addr_alias = g_local_addr_tab[index1];
                local_link->local_addr_mask = g_local_mask_tab[index1];
                local_link->dst_alias_ip = node_dst->addr_tab[index2]->addr_ip;

                return;
            }
        }
    }
}



//***********************************************************************
//***********************************************************************


void tc_add_node(ip_v4_addr addr_ip)
{
    struct olsr_node*           new_node;
    struct olsr_node_addr*      new_addr;
    struct olsr_path_buffer*    buff;


    // allocate all memory items before request TC LOCK
    // because allocation can put the current process to sleep
    // waiting for memory page

    new_node = tc_get_node(addr_ip);
    if(new_node != NULL)
    {
      DBG_MOD(2,"Node %s is already present", inet_ntoa(*(struct in_addr*)&addr_ip)) ;
      return ; // Node already present
    }
    
    new_node = ALLOC_NODE;
    if(new_node == NULL)
    {
        MSG_FAILED("Failed to allocate memory for new tc node" );
        return;
    }
    memset(new_node,0,sizeof(struct olsr_node));


    new_addr = ALLOC_NODE_ADDR;
    if(new_addr == NULL)
    {
        MSG_FAILED("Failed to allocate new node addr");
        FREE_NODE(new_node);
        return;
    }
    memset(new_addr,0,sizeof(struct olsr_node_addr));


    // request write lock
    WRITE_LOCK(g_ser_tc_lock);

    // reset path cache
    buff = g_path_buffer;
    g_path_buffer = NULL;


    if(tc_add_node_addr(new_addr,new_node,addr_ip,2) < 0)
        // failed to add addr element
        goto add_node_failed;

    // add node to global node list
    if(g_tc_node_lst != NULL)
    {
        g_tc_node_lst->prev_node = new_node;
        new_node->next_node = g_tc_node_lst;
    }

    g_tc_node_lst = new_node;

    new_node = NULL;
    new_addr = NULL;

add_node_failed :

    // release write lock
    WRITE_UNLOCK(g_ser_tc_lock);

    if(new_addr != NULL)
        FREE_NODE_ADDR(new_addr);

    if(new_node != NULL)
        FREE_NODE(new_node);

    free_path_buffer(buff);
}



//***********************************************************************
//***********************************************************************


void tc_remove_node(ip_v4_addr addr_ip)
{
    struct olsr_node* node;
    struct olsr_node_link* link, *tmp_link;
    struct olsr_path_buffer* buff;
    struct local_link* local_link, *local_link_tmp;
    struct olsr_node_addr*  addr_tab[MAX_IP_ALIAS_BY_NODE];
    int index;

    local_link = NULL;
    local_link_tmp = NULL;
    buff = NULL;
    link = NULL;
    tmp_link = NULL;
    memset(addr_tab, 0, sizeof(addr_tab));

    // request write lock to update TC data
    WRITE_LOCK(g_ser_tc_lock);

    node = tc_get_node(addr_ip);
    if(node == NULL)
    {
        MSG_FAILED("Failed to find tc node to remove" );
        goto remove_node_failed;
    }

    if(addr_ip == g_local_addr_tab[0])
    {
        // remove local node => remove all local links
        local_link = g_local_links;
        g_local_links = NULL;
    }
    else
    {
        // only remove local link which reach removed node
        local_link = g_local_links;
        while(local_link)
        {
            if(local_link->dst_addr_ip == addr_ip)
            {
                if(local_link_tmp == NULL)
                    // first link
                    g_local_links = local_link->next;
                else
                    local_link_tmp->next = local_link->next;

                local_link->next = NULL;
                break;
            }
            else
            {
                local_link_tmp  = local_link;
                local_link = local_link->next;
            }
        }
    }


    // reset path cache
    buff = g_path_buffer;
    g_path_buffer = NULL;


    // update global node list
    if(node->prev_node == NULL)
        g_tc_node_lst = node->next_node;
    else
        node->prev_node->next_node = node->next_node;

    if(node->next_node != NULL)
        node->next_node->prev_node = node->prev_node;


    // remove node addr
    for(index = 0; index < MAX_IP_ALIAS_BY_NODE; index++)
    {
        if(node->addr_tab[index] != NULL)
            addr_tab[index] = tc_rmv_node_addr(node->addr_tab[index]->addr_ip);
        else
            addr_tab[index] = NULL;
    }


    // check if node still have link
    //------------------------------

    // keep your eyes open !!!!

    // remove links where node is src
    // (ie update ptr of each dst node of all links)
    link = node->link_src;
    while(link)
    {
        if(link->dst_link_prev == NULL)
            link->dst_node->link_dst= link->dst_link_next;
        else
            link->dst_link_prev->dst_link_next= link->dst_link_next;

        if(link->dst_link_next != NULL)
            link->dst_link_next->dst_link_prev = link->dst_link_prev;

        link = link->src_link_next;
    }

    // remove link if node is dst
    link = node->link_dst;
    while(link)
    {
        if(link->src_link_prev == NULL)
            link->src_node->link_src= link->src_link_next;
        else
            link->src_link_prev->src_link_next= link->src_link_next;

        if(link->src_link_next != NULL)
            link->src_link_next->src_link_prev = link->src_link_prev;

        link = link->dst_link_next;
    }


remove_node_failed :

    // release write lock
    WRITE_UNLOCK(g_ser_tc_lock);

    // free local links
    while(local_link)
    {
        local_link_tmp = local_link;
        local_link= local_link->next;
        FREE_LOCAL_LINK(local_link_tmp);
    }

    // free path cache
    free_path_buffer(buff);


    // free node addr
    for(index = 0; index < MAX_IP_ALIAS_BY_NODE; index++)
    {
        if(addr_tab[index] != NULL)
            FREE_NODE_ADDR(addr_tab[index]);
    }

    // free link where node is src
    if (node)
    {
      link=node->link_src;
      while(link)
      {
          tmp_link = link;
          link=link->src_link_next;
          FREE_LINK(tmp_link);
      }
  
      // free link where node is dst
      link=node->link_dst;
      while(link)
      {
          tmp_link = link;
          link=link->dst_link_next;
          FREE_LINK(tmp_link);
      }
      // free the node
      FREE_NODE(node);
    }
}



//***********************************************************************
//***********************************************************************


int tc_init_data(void)
{
    int index;

    for(index =0; index < HASH_TAB_SIZE; index++)
        g_tc_addr_tab[index] = NULL;

    for(index =0; index < MAX_IP_ALIAS_BY_NODE; index++)
    {
        g_local_addr_tab[index] = 0;
        g_local_mask_tab[index] = 0;
    }

#ifdef ALLOC_WITH_CACHE

    g_ser_tc_node_addr_cache = kmem_cache_create("SEREADMO_TC_NODE_ADDR",
                               sizeof(struct olsr_node_addr),0,SLAB_HWCACHE_ALIGN,NULL,NULL);
    if(g_ser_tc_node_addr_cache == NULL)
    {
        MSG_FAILED("Failed to create node addr cached allocation"); 
        return -ENOMEM;
    }

    g_ser_tc_node_cache = kmem_cache_create("SEREADMO_TC_NODE",
                          sizeof(struct olsr_node),0,SLAB_HWCACHE_ALIGN,NULL,NULL);
    if(g_ser_tc_node_cache == NULL)
    {
        MSG_FAILED("Failed to create node cached allocation"); 
        return -ENOMEM;
    }

    g_ser_tc_link_cache = kmem_cache_create("SEREADMO_TC_LINK",
                          sizeof(struct olsr_node_link),0,SLAB_HWCACHE_ALIGN,NULL,NULL);
    if(g_ser_tc_link_cache == NULL)
    {
        MSG_FAILED("Failed to create link cached allocation"); 
        return -ENOMEM;
    }

    g_ser_local_link_cache = kmem_cache_create("SEREADMO_LOCAL_LINK",
                             sizeof(struct local_link),0,SLAB_HWCACHE_ALIGN,NULL,NULL);
    if(g_ser_local_link_cache == NULL)
    {
        MSG_FAILED("Failed to create local link cached allocation"); 
        return -ENOMEM;
    }

    g_ser_tc_path_cache = kmem_cache_create("SEREADMO_PATH",
                          sizeof(struct olsr_path),0,SLAB_HWCACHE_ALIGN,NULL,NULL);
    if(g_ser_tc_path_cache == NULL)
    {
        MSG_FAILED(v"Failed to create path cached allocation"); 
        return -ENOMEM;
    }

    g_ser_tc_path_buffer_cache = kmem_cache_create("SEREADMO_PATH_BUFFER",
                                 sizeof(struct olsr_path_buffer),0,SLAB_HWCACHE_ALIGN,NULL,NULL);
    if(g_ser_tc_path_buffer_cache == NULL)
    {
        MSG_FAILED("Failed to create path buffer cached allocation"); 
        return -ENOMEM;
    }

#endif


    return 0;
}


//***********************************************************************
//***********************************************************************


void tc_free_data(int all)
{
    struct olsr_node        *node, *node_tmp;
    struct olsr_node_link   *link, *link_tmp;
    struct local_link       *local_link_tmp, *local_link;
    struct olsr_node_addr   *node_addr,*node_addr_tmp;
    int index;

    // request write lock
    WRITE_LOCK(g_ser_tc_lock);

    // free all nodes and links
    node = g_tc_node_lst;
    while(node)
    {
        link = node->link_src;
        while(link)
        {
            link_tmp = link;
            link = link->src_link_next;
            FREE_LINK(link_tmp);
        }

        node_tmp = node;
        node = node->next_node;
        FREE_NODE(node_tmp);
    }

    g_tc_node_lst = NULL;

    // free all node addr
    for(index = 0; index < HASH_TAB_SIZE; index++)
    {
        node_addr = g_tc_addr_tab[index];
        while(node_addr)
        {
            node_addr_tmp = node_addr;
            node_addr = node_addr->hash_next;
            FREE_NODE_ADDR(node_addr_tmp);
        }

        g_tc_addr_tab[index] = NULL;
    }

    free_path_buffer(g_path_buffer);
    g_path_buffer = NULL;

    local_link = g_local_links;
    while(local_link)
    {
        local_link_tmp = local_link;
        local_link=local_link->next;
        FREE_LOCAL_LINK(local_link_tmp);
    }
    g_local_links =NULL;


    for(index =0; index < MAX_IP_ALIAS_BY_NODE; index++)
    {
        g_local_addr_tab[index] = 0;
        g_local_mask_tab[index] = 0;
    }

#ifdef ALLOC_WITH_CACHE

    if(all)
    {
        if(g_ser_tc_node_addr_cache)
            kmem_cache_destroy(g_ser_tc_node_addr_cache);

        if(g_ser_tc_link_cache)
            kmem_cache_destroy(g_ser_tc_link_cache);

        if(g_ser_tc_node_cache)
            kmem_cache_destroy(g_ser_tc_node_cache);

        if(g_ser_local_link_cache)
            kmem_cache_destroy(g_ser_local_link_cache);

        if(g_ser_tc_path_cache)
            kmem_cache_destroy(g_ser_tc_path_cache);

         if(g_ser_tc_path_buffer_cache)
            kmem_cache_destroy(g_ser_tc_path_buffer_cache);
    }

#endif

    WRITE_UNLOCK(g_ser_tc_lock);
}

//***********************************************************************
//***********************************************************************


void tc_add_alias(ip_v4_addr addr_ip, ip_v4_addr addr_alias)
{
    struct olsr_node* node;
    struct local_link* local_link;
    struct olsr_node_addr*      new_addr;


    // alias modification doesn't change TC (nodes, links)
    // => no need to clean path cache

    // allocate all memory items before request TC LOCK
    // because allocation can put the current process to sleep
    // waiting for memory page

    new_addr = ALLOC_NODE_ADDR;
    if(new_addr == NULL)
    {
        MSG_FAILED("Failed to allocate new node addr");
        return;
    }
    memset(new_addr,0,sizeof(struct olsr_node_addr));


    // request write lock to update TC data
    WRITE_LOCK(g_ser_tc_lock);

    // add or remove alias don't change TC => don't clean cache path
    node = tc_get_node(addr_ip);
    if(node == NULL)
    {
        MSG_FAILED("Failed to add node alias : %lX -> %lX", addr_ip,addr_alias);
        goto add_alias_failed;
    }

    tc_add_node_addr(new_addr,node,addr_alias,0);
    new_addr = NULL;

    // update locals links
    local_link = g_local_links;
    while(local_link)
    {
        if(local_link->dst_addr_ip == addr_ip)
        {
            tc_update_local_link(node,local_link);
            break;
        }
        local_link = local_link->next;
    }


add_alias_failed:

    // release write lock
    WRITE_UNLOCK(g_ser_tc_lock);

    if(new_addr != NULL)
        FREE_NODE_ADDR(new_addr);
}


//***********************************************************************
//***********************************************************************


void tc_remove_alias(ip_v4_addr addr_ip, ip_v4_addr addr_alias)
{
    struct olsr_node* node;
    struct local_link* local_link;
    struct olsr_node_addr*  node_addr = NULL;


    // request write lock to update TC data
    WRITE_LOCK(g_ser_tc_lock);

    // add or remove alias don't change TC => no need to clean path cache
    node = tc_get_node(addr_ip);
    if(node == NULL)
    {
        MSG_FAILED("Failed to remove node alias : %lX -> %lX", addr_ip,addr_alias);
        goto remove_alias_failed;
    }

    node_addr = tc_rmv_node_addr(addr_alias);

    // update locals links
    local_link = g_local_links;
    while(local_link)
    {
        if(local_link->dst_addr_ip == addr_ip)
        {
            tc_update_local_link(node,local_link);
            break;
        }
        local_link = local_link->next;
    }

remove_alias_failed :

    // release write lock
    WRITE_UNLOCK(g_ser_tc_lock);

    if(node_addr != NULL)
        FREE_NODE_ADDR(node_addr);

}


//***********************************************************************
//***********************************************************************


void tc_set_local_addr(ip_v4_addr addr_ip, ip_v4_addr mask)
{
    WRITE_LOCK(g_ser_tc_lock);

    g_local_addr_tab[0] = addr_ip;
    g_local_mask_tab[0] = mask;

    WRITE_UNLOCK(g_ser_tc_lock);
}

//***********************************************************************
//***********************************************************************


void tc_set_local_alias(ip_v4_addr addr_ip, ip_v4_addr mask)
{
    int index;

    WRITE_LOCK(g_ser_tc_lock);

    for(index =1; index < MAX_IP_ALIAS_BY_NODE; index++)
    {
        if(g_local_addr_tab[index] == 0)
        {
            g_local_addr_tab[index] = addr_ip;
            g_local_mask_tab[index] = mask;
            break;
        }
    }

    WRITE_UNLOCK(g_ser_tc_lock);
}


//***********************************************************************
//***********************************************************************


void tc_add_link(ip_v4_addr addr_src, ip_v4_addr addr_dst, int cost)
{
    struct olsr_node*           node_dst;
    struct olsr_node*           node_src;
    struct olsr_node_link*      link = NULL;
    struct olsr_path_buffer*    buff;
    struct local_link*          local_link = NULL;


    // allocate memory for new link
    //-----------------------------

    // use cached allocation to optimise memory acces
    link = ALLOC_LINK;
    if(link == NULL)
    {
        MSG_FAILED("Failed to allocate memory for new tc link" );
        return;
    }
    memset(link,0,sizeof(struct olsr_node_link));


    if(addr_src == g_local_addr_tab[0])
    {
        local_link = ALLOC_LOCAL_LINK;
        if(local_link == NULL)
        {
            MSG_FAILED("Failed to allocate memory for new tc link" );
            FREE_LINK(link);
            return;
        }
        memset(local_link, 0, sizeof(struct local_link));
    }


    // request write lock to update TC data
    WRITE_LOCK(g_ser_tc_lock);

    // reset path cache
    buff = g_path_buffer;
    g_path_buffer = NULL;

    //find src node
    //-------------
    node_src = tc_get_node(addr_src);
    if(node_src == NULL)
    {
        MSG_FAILED("Failed to find src node for new tc link" );
        goto link_failed;
    }

    //find dst node
    //-------------
    node_dst = tc_get_node(addr_dst);
    if(node_dst == NULL)
    {
        MSG_FAILED( "Failed to find dst node for new tc link" );
        goto link_failed;
    }

    // if src_node is local machine add new local link
    if(local_link != NULL)
    {
        local_link->next = g_local_links;
        g_local_links = local_link;
        tc_update_local_link(node_dst, local_link);
        local_link = NULL;
    }

    // add link to src node
    //---------------------
    link->cost = cost;
    link->src_node = node_src;
    if(node_src->link_src)
        node_src->link_src->src_link_prev = link;

    link->src_link_next = node_src->link_src;
    node_src->link_src = link;


    // add link to dst node
    //---------------------
    link->dst_node = node_dst;
    if(node_dst->link_dst)
        node_dst->link_dst->dst_link_prev = link;

    link->dst_link_next = node_dst->link_dst;
    node_dst->link_dst = link;

    link = NULL;

link_failed:

    // release write lock
    WRITE_UNLOCK(g_ser_tc_lock);

    free_path_buffer(buff);

    if(link != NULL)
        FREE_LINK(link);

    if(local_link)
        FREE_LOCAL_LINK(local_link);

}

//***********************************************************************
//***********************************************************************

void tc_remove_link(ip_v4_addr addr_src, ip_v4_addr addr_dst)
{
    struct olsr_node*           node_src;
    struct olsr_node_link*      link = NULL;
    struct olsr_path_buffer*    buff=NULL;
    struct local_link*          local_link, *local_link_prev;

    local_link = NULL;

    // request write lock to update TC data
    WRITE_LOCK(g_ser_tc_lock);

    // reset path cache
    buff = g_path_buffer;
    g_path_buffer = NULL;


    //find src node
    //-------------
    node_src = tc_get_node(addr_src);
    if(node_src == NULL)
    {
        MSG_FAILED("Failed to find src node to remove tc link" );
        goto link_failed;
    }

    //find link
    //---------
    link = node_src->link_src;
    while(link)
    {
        if(link->dst_node->addr_tab[0]->addr_ip == addr_dst)
            break;

        link = link->src_link_next;
    }

    if(link == NULL)
    {
        MSG_FAILED("Failed to find tc link to remove" );
        goto link_failed;
    }

    // update linked lists
    //--------------------

    // keep your eyes open !!!!

    if(link->dst_link_prev == NULL)
        link->dst_node->link_dst= link->dst_link_next;
    else
        link->dst_link_prev->dst_link_next= link->dst_link_next;

    if(link->dst_link_next != NULL)
        link->dst_link_next->dst_link_prev = link->dst_link_prev;

    if(link->src_link_prev == NULL)
        link->src_node->link_src= link->src_link_next;
    else
        link->src_link_prev->src_link_next= link->src_link_next;

    if(link->src_link_next != NULL)
        link->src_link_next->src_link_prev = link->src_link_prev;

    local_link = NULL;
    if(addr_src == g_local_addr_tab[0])
    {
        local_link_prev=NULL;
        local_link = g_local_links;
        while(local_link)
        {
            if(local_link->dst_addr_ip == addr_dst)
            {
                if(local_link_prev == NULL)
                    // first link
                    g_local_links = local_link->next;
                else
                    local_link_prev->next = local_link->next;

                break;
            }
            local_link_prev  = local_link;
            local_link = local_link->next;
        }
    }

link_failed:

    // release write lock
    WRITE_UNLOCK(g_ser_tc_lock);

    free_path_buffer(buff);

    if(link != NULL)
        FREE_LINK(link);

    if(local_link)
        FREE_LOCAL_LINK(local_link);
}

