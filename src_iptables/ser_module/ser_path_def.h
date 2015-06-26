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
*
*
*************************************************************************
* Changes :
* --------
*
*************************************************************************
* Used and modified 2015
*   by      Benjamin Mollé (engineering student, Polytech Nantes, University of Nantes, France) benjamin.molle@gmail.com
*           Denis Souron (engineering student, Polytech Nantes, Université of Nantes, France) denis.souron@laposte.net
*************************************************************************/
#ifndef _SER_PATH_DEF_H
#define _SER_PATH_DEF_H


#ifndef APP_TEST

    // kernel module include
    #include <linux/string.h>
    #include <linux/netfilter_ipv4.h>
    #include <linux/spinlock.h>
    #include <linux/errno.h>

    #ifdef ALLOC_WITH_CACHE

        #define ALLOC_NODE_ADDR         (struct olsr_node_addr*) kmem_cache_alloc(g_ser_tc_node_addr_cache,GFP_ATOMIC)
        #define ALLOC_NODE              (struct olsr_node*) kmem_cache_alloc(g_ser_tc_node_cache,GFP_ATOMIC)
        #define ALLOC_LINK              (struct olsr_node_link*) kmem_cache_alloc(g_ser_tc_link_cache,GFP_ATOMIC)
        #define ALLOC_LOCAL_LINK        (struct local_link*) kmem_cache_alloc(g_ser_local_link_cache,GFP_ATOMIC)
        #define ALLOC_PATH              (struct olsr_path*) kmem_cache_alloc(g_ser_tc_path_cache,GFP_ATOMIC)
        #define ALLOC_PATH_BUFFER       (struct olsr_path_buffer*) kmem_cache_alloc(g_ser_tc_path_buffer_cache,GFP_ATOMIC)

        #define FREE_NODE_ADDR(addr)    kmem_cache_free(g_ser_tc_node_addr_cache, addr)
        #define FREE_NODE(node)         kmem_cache_free(g_ser_tc_node_cache, node)
        #define FREE_LINK(link)         kmem_cache_free(g_ser_tc_link_cache, link)
        #define FREE_LOCAL_LINK(link)   kmem_cache_free(g_ser_local_link_cache, link)
        #define FREE_PATH(path)         kmem_cache_free(g_ser_tc_path_cache, path)
        #define FREE_PATH_BUFFER(path)  kmem_cache_free(g_ser_tc_path_buffer_cache, path)

    #else

        #define ALLOC_NODE_ADDR         (struct olsr_node_addr*) kmalloc(sizeof(struct olsr_node_addr),GFP_ATOMIC)
        #define ALLOC_NODE              (struct olsr_node*) kmalloc(sizeof(struct olsr_node),GFP_ATOMIC)
        #define ALLOC_LINK              (struct olsr_node_link*) kmalloc(sizeof(struct olsr_node_link),GFP_ATOMIC)
        #define ALLOC_LOCAL_LINK        (struct local_link*) kmalloc(sizeof(struct local_link),GFP_ATOMIC)
        #define ALLOC_PATH              (struct olsr_path*) kmalloc(sizeof(struct olsr_path),GFP_ATOMIC)
        #define ALLOC_PATH_BUFFER       (struct olsr_path_buffer*) kmalloc(sizeof(struct olsr_path_buffer),GFP_ATOMIC)

        #define FREE_NODE_ADDR(addr)    kfree(addr)
        #define FREE_NODE(node)         kfree(node)
        #define FREE_LINK(link)         kfree(link)
        #define FREE_LOCAL_LINK(link)   kfree(link)
        #define FREE_PATH(path)         kfree(path)
        #define FREE_PATH_BUFFER(path)  kfree(path)

    #endif


    #define ALLOC_PATH_DATA(size)       (ip_v4_addr*) kmalloc(size,GFP_ATOMIC)
    #define FREE_PATH_DATA(path)        kfree(path)

    extern rwlock_t                     g_ser_tc_lock;

#else

    // user level application
    #include <stdlib.h>
    #include <stdio.h>
    #include <pthread.h>
    #include <string.h>
    #include <netinet/in.h>

    #define ALLOC_NODE_ADDR         (struct olsr_node_addr*) malloc(sizeof(struct olsr_node_addr))
    #define ALLOC_NODE              (struct olsr_node*) malloc(sizeof(struct olsr_node))
    #define ALLOC_LINK              (struct olsr_node_link*) malloc(sizeof(struct olsr_node_link))
    #define ALLOC_LOCAL_LINK        (struct local_link*) malloc(sizeof(struct local_link))
    #define ALLOC_PATH              (struct olsr_path*) malloc(sizeof(struct olsr_path))
    #define ALLOC_PATH_BUFFER       (struct olsr_path_buffer*) malloc(sizeof(struct olsr_path_buffer))
    #define ALLOC_PATH_DATA(size)   (ip_v4_addr*) malloc(size);

    #define FREE_NODE_ADDR(addr)    free(addr)
    #define FREE_NODE(node)         free(node)
    #define FREE_LINK(link)         free(link)
    #define FREE_LOCAL_LINK(link)   free(link)
    #define FREE_PATH(path)         free(path)
    #define FREE_PATH_BUFFER(path)  free(path)
    #define FREE_PATH_DATA(path)    free(path)

    extern pthread_mutex_t          g_ser_tc_lock;

#endif


#define MAX_IP_ALIAS_BY_NODE        5
#define HASH_TAB_SIZE               256


extern struct olsr_node_addr*      g_tc_addr_tab[HASH_TAB_SIZE];
extern struct olsr_node*           g_tc_node_lst;
extern struct olsr_path_buffer*    g_path_buffer;
extern ip_v4_addr                  g_local_addr_tab[MAX_IP_ALIAS_BY_NODE];
extern ip_v4_addr                  g_local_mask_tab[MAX_IP_ALIAS_BY_NODE];
extern struct local_link*          g_local_links;




struct olsr_node_addr
{
    ip_v4_addr              addr_ip;        // node IP address
    int                     addr_pos;

    struct olsr_node*       node;

    struct olsr_node_addr*  hash_next;      // next node with same hash key in g_tc_addr_tab table
    struct olsr_node_addr*  hash_prev;      // next node with same hash key in g_tc_addr_tab table
};


struct olsr_node
{
    struct olsr_node_addr*  addr_tab[MAX_IP_ALIAS_BY_NODE];
    struct olsr_node_link*  link_src;       // linked list of links !!! where current node is src node
    struct olsr_node_link*  link_dst;       // linked list of links !!! where current node is dst node

    struct olsr_node*       next_node;
    struct olsr_node*       prev_node;

    unsigned long           dijkstra_cost;
    struct olsr_node*       dijkstra_list;
    struct olsr_node*       dijkstra_path;
    int                     dijkstra_hops;
    int                     dijkstra_processed;
};

struct olsr_node_link
{
    struct olsr_node*       src_node;
    struct olsr_node*       dst_node;
    unsigned long           cost;               // initial link cost
    unsigned long           dijkstra_cost;      // link cost updated by k_dijkstra
    unsigned long           dijkstra_cost_p1;   // link cost updated by k_dijkstra

    struct olsr_node_link*  src_link_next;
    struct olsr_node_link*  src_link_prev;

    struct olsr_node_link*  dst_link_next;
    struct olsr_node_link*  dst_link_prev;

};


struct olsr_path
{
    ip_v4_addr*             path;
    int                     path_hops;
    struct olsr_path*       next;
};

// entry for path cache
// each entry store all path from src to dst node
struct olsr_path_buffer
{
    ip_v4_addr                  src_addr_ip;    // main IP of src node
    ip_v4_addr                  dst_addr_ip;    // main IP of dst node

    struct olsr_path*           lst_path;       // linked list of path

    struct olsr_path_buffer*    next;           // linked list of cache path entry
};

struct local_link
{
    ip_v4_addr               dst_addr_ip;
    ip_v4_addr               dst_alias_ip;
    ip_v4_addr               local_addr_alias;
    ip_v4_addr               local_addr_mask;

    struct local_link*       next;
};


#define FP(value)                   (value*g_conf_fp);
#define FE(value)                   (value*g_conf_fe);


struct olsr_path*   tc_k_dijstra(ip_v4_addr srcAddr, ip_v4_addr dstAddr);
void                tc_print_path(struct olsr_path*  path, int dbg_level);

#endif //_SER_PATH_DEF_H

