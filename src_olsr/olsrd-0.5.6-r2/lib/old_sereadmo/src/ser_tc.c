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

//****************************************************************************
//****************************************************************************

//check if addr is from local machine
static int tc_is_addr_local(union olsr_ip_addr* addr)
{
    struct interface* ifs;
    struct ipaddr_str addrbuf;

    for(ifs = ifnet; ifs != NULL; ifs = ifs->int_next)
        if(ipequal(addr,&ifs->ip_addr) != 0)
            return 1;

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
        SER_PRINTF("Test node Eq : %X - %X\n",addr->v4.s_addr,node->src_addr.v4.s_addr);
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

    SER_PRINTF("Create node : %X\n",addr->v4.s_addr);

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
            return 1;
    }
    OLSR_FOR_ALL_NBR_ENTRIES_END(neigh)

    return 0;
}



//****************************************************************************
//****************************************************************************

static void tc_reset_active_flag(void)
{
    int index1, index2;
    struct ser_tc_entry*       entry_node;
    struct ser_tc_entry_dst*   dst_node;

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

    SER_PRINTF("Check link : %X -> %X\n",src_addr->v4.s_addr,dst_addr->v4.s_addr);

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

    // prise en compte du bug des liens voisins
    if(src->is_src_local == 1)
    {
        // le lien est un acces direct vers un voisin
        // => verif de l'existance du voisin
        if(!tc_is_in_neighbourhood(dst_addr))
            // le dst n'est pas parmi les voisins => bug olsrd
            return;
    }
    
    // on indique que le noeud est actif
    src->is_active = 1;
    

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

        SER_PRINTF("Create link : %X -> %X\n",src_addr->v4.s_addr,dst_addr->v4.s_addr);

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
        SER_PRINTF("Validate link : %X -> %X\n",src_addr->v4.s_addr,dst_addr->v4.s_addr);
    }

    dst_link->is_active = 1;
}


//****************************************************************************
//****************************************************************************


static void tc_remove_useless_link(void)
{
    int index1, index2;
    struct ser_tc_entry*       entry_node, *tmp_entry;
    struct ser_tc_entry_dst*   dst_node, *tmp_dst;

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
                    printf("ALERT : suppression d'un noeud source ayant encore des destinataies");

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

    tc_reset_active_flag();

    OLSR_FOR_ALL_TC_ENTRIES(tc) 
    {
        OLSR_FOR_ALL_TC_EDGE_ENTRIES(tc, tc_edge) 
        {
            tc_addLink(&(tc->addr),&(tc_edge->T_dest_addr));
        }
        OLSR_FOR_ALL_TC_EDGE_ENTRIES_END(tc, tc_edge);
    }
    OLSR_FOR_ALL_TC_ENTRIES_END(tc);

    tc_remove_useless_link();

}


//****************************************************************************
//****************************************************************************



void tc_init_data(void)
{
    int index;

    for(index = 0; index < HASHSIZE; index++)
        g_cur_ser_tc_tab[index]=NULL;

}

