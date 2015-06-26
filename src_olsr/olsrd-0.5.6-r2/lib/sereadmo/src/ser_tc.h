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

#ifndef _SER_TC
#define _SER_TC

#include "olsr_types.h"
#include "hashing.h"

struct ser_tc_mid
{
    union olsr_ip_addr          main_addr;          // main IP addr
    union olsr_ip_addr          alias_addr;         // alias IP addr

    int                         is_active;          // mid is used

    struct ser_tc_mid*          next;
};



struct ser_tc_entry_dst
{
    union olsr_ip_addr          dst_addr;           // neighbour node IP addr

    struct ser_tc_entry_dst*    hash_next;          // next link
    struct ser_tc_entry_dst*    hash_prev;          // previous link

    int                         is_active;          // link is used (ie present in OLSR TC)
};



struct ser_tc_entry
{
    union olsr_ip_addr          src_addr;           // current node IP addr

    struct ser_tc_entry*        hash_prev;          // next node with same HASH key of IP addr
    struct ser_tc_entry*        hash_next;          // next node with same HASH key of IP addr

    struct ser_tc_entry_dst*    dst_tab[HASHSIZE];  // links from current node to neighbour nodes
                                                    // (linked list of kinks ....)

    int                         nb_dst;             // nb of links (just for check during memory cleanup)

    int                         is_active;            // node is used (ie present in OLSR TC)
    int                         is_src_local;        // this node is the local machine node
};


// all node in the network
extern struct ser_tc_entry* g_cur_ser_tc_tab[HASHSIZE];

extern struct ser_tc_mid* g_cur_mid_tc;

// called when OLSRD change its TC
void tc_check_data(void);

// called when OLSRD change its TC
void tc_send_all_data(void);

// called at startup to initialise memory
void tc_init_data(void);

#endif

