
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
*************************************************************************/
#ifndef _SER_TC
#define _SER_TC

#include "olsr_types.h"
#include "hashing.h"


struct ser_tc_entry_dst
{
    union olsr_ip_addr          dst_addr;			// neighbour node IP addr

    struct ser_tc_entry_dst*    hash_next;			// next link
    struct ser_tc_entry_dst*    hash_prev;			// previous link

    int                         is_active;			// link is used (ie present in OLSR TC)
};



struct ser_tc_entry
{
    union olsr_ip_addr          src_addr;           // current node IP addr

    struct ser_tc_entry*        hash_prev;          // next node with same HASH key of IP addr
    struct ser_tc_entry*        hash_next;          // next node with same HASH key of IP addr

    struct ser_tc_entry_dst*    dst_tab[HASHSIZE];  // links from current node to neighbour nodes
                                                    // (linked list of kinks ....)

    int                         nb_dst;             // nb of links (just for check during memory cleanup)

    int                         is_active;			// node is used (ie present in OLSR TC)
    int                         is_src_local;		// this node is the local machine node
};


// all node in the network
extern struct ser_tc_entry* g_cur_ser_tc_tab[HASHSIZE];

// called when OLSRD change its TC
void tc_check_data(void);

// called at startup to initialise memory
void tc_init_data(void);

#endif

