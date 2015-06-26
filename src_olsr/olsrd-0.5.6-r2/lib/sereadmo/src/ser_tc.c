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

#include <stdio.h>
#include "ipcalc.h"
#include "tc_set.h"
#include "ser_tc.h"
#include "neighbor_table.h"
#include "mid_set.h"
#include "ser_print.h"
#include "ser_device.h"
#include "ser_global.h"


struct ser_tc_entry* g_cur_ser_tc_tab[HASHSIZE];
struct ser_tc_mid*   g_cur_mid_tc;

//****************************************************************************
//****************************************************************************

//check if addr is from local machine
static int tc_is_addr_local(union olsr_ip_addr* addr)
{
    struct interface* ifs;
    struct ipaddr_str addrbuf;

    SER_PRINTF("tc_is_addr_local : processing node %s", inet_ntoa(addr->v4));

    for(ifs = ifnet; ifs != NULL; ifs = ifs->int_next)
    {
        SER_PRINTF("tc_is_addr_local : comparing to local interface  %s", inet_ntoa(ifs->ip_addr.v4));
        if(ipequal(addr,&ifs->ip_addr) != 0)
        {
            SER_PRINTF("tc_is_addr_local : addr  %s is local", inet_ntoa(addr->v4));
            return 1;
        }
    }

    SER_PRINTF("tc_is_addr_local : addr  %s is not local", inet_ntoa(addr->v4));
    return 0;
}


//****************************************************************************
//****************************************************************************


static struct ser_tc_entry* tc_get_node(union olsr_ip_addr* addr)
{
    struct ser_tc_entry* node;
    olsr_u32_t hash = olsr_ip_hashing(addr);

    node = g_cur_ser_tc_tab[hash];
    while(node)
    {
        //SER_PRINTF("Test node Eq : %X - %X\n",addr->v4.s_addr,node->src_addr.v4.s_addr);
        if(ipequal(addr,&(node->src_addr)) != 0)
            break;
        node = node->hash_next;
    }

    return node;
}

//****************************************************************************
//****************************************************************************

static struct ser_tc_entry* tc_create_node(union olsr_ip_addr* addr)
{
    struct ser_tc_entry* node, *pos;
    olsr_u32_t hash = olsr_ip_hashing(addr);

    //SER_PRINTF("Create node : %X\n",addr->v4.s_addr);

    node = malloc(sizeof(struct ser_tc_entry));
    memset(node,0,sizeof(struct ser_tc_entry));    

    genipcopy(&node->src_addr,addr);
    node->is_active = 1;
    node->is_src_local = tc_is_addr_local(addr);

    memset(node->dst_tab,0,HASHSIZE * sizeof(struct ser_tc_entry_dst*));

    pos = g_cur_ser_tc_tab[hash];
    if(pos != NULL)
    {
        pos->hash_prev = node;
        node->hash_next = pos;
    }
    g_cur_ser_tc_tab[hash] = node;

    dev_add_node(addr);

    return node;
}

//****************************************************************************
//****************************************************************************

static int tc_is_in_neighbourhood(union olsr_ip_addr* addr)
{
    struct neighbor_entry *neigh;

    OLSR_FOR_ALL_NBR_ENTRIES(neigh)
    {
        if(ipequal(addr,&neigh->neighbor_main_addr) != 0)
        {
            if(neigh->status == SYM)
            {
              SER_PRINTF("tc_is_in_neighbourhood : node %s is a SYMETRIC neighbor", inet_ntoa(addr->v4));
              return 1;
            }
            else
            {
              SER_PRINTF("tc_is_in_neighbourhood : node %s is a NON SYMETRIC neighbor", inet_ntoa(addr->v4));
              return 0;
            }
        }
    }
    OLSR_FOR_ALL_NBR_ENTRIES_END(neigh)

    SER_PRINTF("tc_is_in_neighbourhood : node %s is not a neighbor", inet_ntoa(addr->v4));
    return 0;
}



//****************************************************************************
//****************************************************************************

static void tc_reset_active_flag(void)
{
    int index1, index2;
    struct ser_tc_entry*       entry_node;
    struct ser_tc_entry_dst*   dst_node;
    struct ser_tc_mid*         mid_entry;

    for(index1 = 0; index1 < HASHSIZE; index1++)
    {
        entry_node = g_cur_ser_tc_tab[index1];
        while(entry_node)
        {
            entry_node->is_active = 0;
            for(index2 = 0; index2 < HASHSIZE; index2++)
            {
                dst_node = entry_node->dst_tab[index2];
                while(dst_node)
                {
                    dst_node->is_active = 0;
                    dst_node = dst_node->hash_next;
                }
            }

            entry_node = entry_node->hash_next;
        }
    }

    mid_entry = g_cur_mid_tc;
    while(mid_entry)
    {
        mid_entry->is_active = 0;
        mid_entry = mid_entry->next;
    }
}


//****************************************************************************
//****************************************************************************



static void tc_addLink(union olsr_ip_addr* src_addr, union olsr_ip_addr* dst_addr)
{
    struct ser_tc_entry       *dst, *src;
    struct ser_tc_entry_dst*   dst_link_tmp, *dst_link;


    olsr_u32_t hash_dst = olsr_ip_hashing(dst_addr);

    // check for src and dst node and create them if needed
    //-----------------------------------------------------

    //SER_PRINTF("Check link : %X -> %X\n",src_addr->v4.s_addr,dst_addr->v4.s_addr);

    src = tc_get_node(src_addr);
    if(src == NULL)
    {
        // attention la mehode tc_is_addr_local est longue car elle 
        // verifie la config des cartes reseau OLSR

        // prise en compte du bug des liens voisins
        if(tc_is_addr_local(src_addr))
        {
            // le lien est un acces direct vers un voisin
            // => verif de l'existance du voisin
            if(!tc_is_in_neighbourhood(dst_addr))
                // le dst n'est pas parmi les voisins => bug olsrd
                return;
        }

        // create missing node
        src = tc_create_node (src_addr);
        src->is_active = 1;
    }
    else
    {
        // prise en compte du bug des liens voisins
        if(src->is_src_local == 1)
        {
            // le lien est un acces direct vers un voisin
            // => verif de l'existance du voisin
            if(!tc_is_in_neighbourhood(dst_addr))
                // le dst n'est pas parmi les voisins => bug olsrd
                return;
        }

        src->is_active = 1;
    }

    // on indique que le noeud est actif


    // check dst node
    //---------------
    dst = tc_get_node(dst_addr);
    // create missing node if needed
    if(dst == NULL)  dst = tc_create_node (dst_addr);
    dst->is_active = 1;


    // check for link between src and dst
    //-----------------------------------
    dst_link = NULL;
    dst_link_tmp = src->dst_tab[hash_dst];
    while(dst_link_tmp)
    {
        if(ipequal(dst_addr,&(dst_link_tmp->dst_addr)) != 0)
        {
            dst_link = dst_link_tmp;
            break;
        }
        dst_link_tmp = dst_link_tmp->hash_next;
    }

    if(dst_link == NULL)
    {

        //SER_PRINTF("Create link : %X -> %X\n",src_addr->v4.s_addr,dst_addr->v4.s_addr);

        dst_link = malloc(sizeof(struct ser_tc_entry_dst));
        genipcopy(&(dst_link->dst_addr),dst_addr);
        dst_link->hash_next = NULL;
        dst_link->hash_prev = NULL;

        dst_link_tmp = src->dst_tab[hash_dst];
        if(dst_link_tmp != NULL)
        {
            dst_link_tmp->hash_prev = dst_link;
            dst_link->hash_next = dst_link_tmp;

        }
        src->dst_tab[hash_dst] = dst_link;
        src->nb_dst++;

        dev_add_link(src_addr, dst_addr,1);
    }
    else
    {
        //SER_PRINTF("Validate link : %X -> %X\n",src_addr->v4.s_addr,dst_addr->v4.s_addr);
    }

    dst_link->is_active = 1;
}


//****************************************************************************
//****************************************************************************


static void tc_addMid(union olsr_ip_addr* main_addr, union olsr_ip_addr* alias_addr)
{
    struct ser_tc_mid*         mid_entry;

    mid_entry = g_cur_mid_tc;
    while(mid_entry)
    {
        if((ipequal(main_addr,&mid_entry->main_addr) != 0) && 
           (ipequal(alias_addr,&mid_entry->alias_addr) != 0))
        {
            // MID entry already exits
            mid_entry->is_active = 1;
            return;
        }
        mid_entry = mid_entry->next;
    }

    //MID entry doesn't exist
    mid_entry = malloc(sizeof(struct ser_tc_mid));
    memset(mid_entry,0,sizeof(struct ser_tc_mid));

    genipcopy(&mid_entry->main_addr,main_addr);
    genipcopy(&mid_entry->alias_addr,alias_addr);
    mid_entry->is_active = 1;
    mid_entry->next = g_cur_mid_tc;
    g_cur_mid_tc = mid_entry;

    dev_add_alias(main_addr,alias_addr);
}

//****************************************************************************
//****************************************************************************


static void tc_remove_useless_mid(void)
{
    struct ser_tc_mid*   mid_entry, *prev_mid;

    mid_entry = g_cur_mid_tc;
    prev_mid = NULL;
    while(mid_entry)
    {
        if(mid_entry->is_active == 0)
        {
            dev_rmv_alias(&mid_entry->main_addr,&mid_entry->alias_addr);
            if(prev_mid == NULL)
                g_cur_mid_tc = mid_entry->next;
            else
                prev_mid->next = mid_entry->next;
        }
        else
        {
            prev_mid = mid_entry;
        }

        mid_entry = mid_entry->next;
    }
}


//****************************************************************************
//****************************************************************************


static void tc_remove_useless_link(void)
{
    int index1, index2;
    struct ser_tc_entry*       entry_node, *tmp_entry;
    struct ser_tc_entry_dst*   dst_node, *tmp_dst;
    struct ser_tc_mid*         mid_entry, *prev_mid;

    for(index1 = 0; index1 < HASHSIZE; index1++)
    {
        entry_node = g_cur_ser_tc_tab[index1];
        while(entry_node)
        {

            for(index2 = 0; index2 < HASHSIZE; index2++)
            {

                dst_node = entry_node->dst_tab[index2];
                while(dst_node)
                {
                    if(dst_node->is_active == 0)
                    {
                        if(dst_node->hash_next)
                            dst_node->hash_next->hash_prev = dst_node->hash_prev;

                        if(dst_node->hash_prev)
                        {
                            dst_node->hash_prev->hash_next = dst_node->hash_next;
                        }
                        else
                        {
                            entry_node->dst_tab[index2] = dst_node->hash_next;
                        }

                        dev_rmv_link(&entry_node->src_addr, &dst_node->dst_addr);

                        entry_node->nb_dst--;
                        tmp_dst = dst_node;
                        dst_node = dst_node->hash_next;

                        free(tmp_dst);
                    }
                    else
                    {
                        dst_node = dst_node->hash_next;
                    }
                }
            }


            if(entry_node->is_active == 0)
            {
                if(entry_node->nb_dst != 0)
                    printf("ALERT : suppression d'un noeud source ayant encore des destinataires\n");

                mid_entry = g_cur_mid_tc;
                prev_mid = NULL;
                while(mid_entry)
                {
                    if(ipequal(&mid_entry->main_addr,&entry_node->src_addr) != 0)
                    {
                        dev_rmv_alias(&mid_entry->main_addr,&mid_entry->alias_addr);
                        if(prev_mid == NULL)
                            g_cur_mid_tc = mid_entry->next;
                        else
                            prev_mid->next = mid_entry->next;
                    }
                    else
                    {
                        prev_mid = mid_entry;
                    }

                    mid_entry = mid_entry->next;
                }

                if(entry_node->hash_next)
                    entry_node->hash_next->hash_prev = entry_node->hash_prev;

                if(entry_node->hash_prev)
                {
               }
                else
                {
                    g_cur_ser_tc_tab[index1] = entry_node->hash_next;
                }

                dev_rmv_node(&entry_node->src_addr);

                tmp_entry = entry_node;
                entry_node = entry_node->hash_next;
                free(tmp_entry);
            }
            else
            {
                entry_node = entry_node->hash_next;
            }
        }
    }

}



//****************************************************************************
//****************************************************************************



void tc_check_data(void)
{
    struct tc_entry *tc;
    struct tc_edge_entry *tc_edge;
    int idx;

    tc_reset_active_flag();

    OLSR_FOR_ALL_TC_ENTRIES(tc) 
    {
        //SER_PRINTF("tc_check_data : processing node %s", inet_ntoa(tc->addr.v4));
        OLSR_FOR_ALL_TC_EDGE_ENTRIES(tc, tc_edge) 
        {
            tc_addLink(&(tc->addr),&(tc_edge->T_dest_addr));
        }
        OLSR_FOR_ALL_TC_EDGE_ENTRIES_END(tc, tc_edge);
    }
    OLSR_FOR_ALL_TC_ENTRIES_END(tc);


    for (idx = 0; idx < HASHSIZE; idx++) 
    {
        struct mid_entry *tmp_list = mid_set[idx].next;

        /*Traverse MID list */
        for (tmp_list = mid_set[idx].next; tmp_list != &mid_set[idx];tmp_list = tmp_list->next) 
        {
            struct mid_address *tmp_addr;

            for (tmp_addr = tmp_list->aliases; tmp_addr; tmp_addr = tmp_addr->next_alias) 
                tc_addMid(&tmp_list->main_addr,&tmp_addr->alias);

        }
    }

    tc_remove_useless_mid();
    tc_remove_useless_link();
}


//****************************************************************************
//****************************************************************************



void tc_init_data(void)
{
    int index;

    for(index = 0; index < HASHSIZE; index++)
        g_cur_ser_tc_tab[index]=NULL;

    g_cur_mid_tc = NULL;
}

//****************************************************************************
//****************************************************************************

void tc_send_all_data(void)
{
    struct tc_entry *tc;
    struct tc_edge_entry *tc_edge;
    int idx;
    static char szIpDst[16];

    //tc_reset_active_flag();
    {
        OLSR_FOR_ALL_TC_ENTRIES(tc)
        {
            SER_PRINTF("tc_send_all_data : adding node %s", inet_ntoa(tc->addr.v4));
            dev_add_node(&(tc->addr));
        }
        OLSR_FOR_ALL_TC_ENTRIES_END(tc);
    }
    {
        OLSR_FOR_ALL_TC_ENTRIES(tc)
        {
            OLSR_FOR_ALL_TC_EDGE_ENTRIES(tc, tc_edge) 
            {		
		  strcpy(szIpDst, inet_ntoa(tc_edge->T_dest_addr.v4));
                  SER_PRINTF("tc_send_all_data : link from %s to %s", inet_ntoa(tc->addr.v4), szIpDst);
                  dev_add_link(&(tc->addr), &(tc_edge->T_dest_addr),1);
            }
            OLSR_FOR_ALL_TC_EDGE_ENTRIES_END(tc, tc_edge);
        }
        OLSR_FOR_ALL_TC_ENTRIES_END(tc);
    }
    
    for (idx = 0; idx < HASHSIZE; idx++) 
    {
        struct mid_entry *tmp_list = mid_set[idx].next;

        //Traverse MID list
        for (tmp_list = mid_set[idx].next; tmp_list != &mid_set[idx];tmp_list = tmp_list->next) 
        {
            struct mid_address *tmp_addr;

            for (tmp_addr = tmp_list->aliases; tmp_addr; tmp_addr = tmp_addr->next_alias) 
                dev_add_alias(&tmp_list->main_addr,&tmp_addr->alias);

        }
    }

}
