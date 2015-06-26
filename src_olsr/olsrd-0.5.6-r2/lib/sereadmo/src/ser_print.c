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

#include "ipcalc.h"
#include "tc_set.h"
#include "neighbor_table.h"
#include "mid_set.h"
#include "ser_print.h"
#include "ser_tc.h"
#include "ser_global.h"


//****************************************************************************
//****************************************************************************

void tc_print_ser_data(void)
{
    int index1, index2;
    struct ser_tc_entry*       entry_node;
    struct ser_tc_entry_dst*   dst_node;
    struct ipaddr_str addrbuf, dstaddrbuf;

    static int index;

    SER_PRINTF("\n\n------------ Topology ------------(%d)\n", index++);

    for(index1 = 0; index1 < HASHSIZE; index1++)
    {
        entry_node = g_cur_ser_tc_tab[index1];
        while(entry_node)
        {
            SER_PRINTF("\n---- src (%d) : %15s \n",entry_node->is_active,
                     olsr_ip_to_string(&addrbuf, &entry_node->src_addr));

            for(index2 = 0; index2 < HASHSIZE; index2++)
            {
                dst_node = entry_node->dst_tab[index2];
                while(dst_node)
                {
                    SER_PRINTF("         dst (%d) : %15s\n",dst_node->is_active,
                              olsr_ip_to_string(&dstaddrbuf, &dst_node->dst_addr));
                    dst_node = dst_node->hash_next;
                }
            }

            entry_node = entry_node->hash_next;
        }
    }

}

//****************************************************************************
//****************************************************************************


void tc_print_olsr_tc(void)
{

    /* The whole function makes no sense without it. */
    struct tc_entry *tc;
    const int ipwidth = olsr_cnf->ip_version == AF_INET ? 15 : 30;

    SER_PRINTF(     "\n---------------------------------------------------- TOPOLOGY\n\n"
                "%-*s %-*s\n", ipwidth, "Source IP addr", ipwidth, "Dest IP addr");

    OLSR_FOR_ALL_TC_ENTRIES(tc) {
        struct tc_edge_entry *tc_edge;

        OLSR_FOR_ALL_TC_EDGE_ENTRIES(tc, tc_edge) {
            struct ipaddr_str addrbuf, dstaddrbuf;
            SER_PRINTF("%-*s %-*s\n",
                        ipwidth, olsr_ip_to_string(&addrbuf, &tc->addr),
                        ipwidth, olsr_ip_to_string(&dstaddrbuf, &tc_edge->T_dest_addr));

        } OLSR_FOR_ALL_TC_EDGE_ENTRIES_END(tc, tc_edge);

    } OLSR_FOR_ALL_TC_ENTRIES_END(tc);

}

//****************************************************************************
//****************************************************************************


void tc_print_olsr_mid(void)
{
    int idx;

    SER_PRINTF("\n---------------------------------------------------- MID\n\n");

    for (idx = 0; idx < HASHSIZE; idx++) {
        struct mid_entry *tmp_list = mid_set[idx].next;

        /*Traverse MID list */
        for (tmp_list = mid_set[idx].next; tmp_list != &mid_set[idx];tmp_list = tmp_list->next) {
            struct mid_address *tmp_addr;
            struct ipaddr_str buf;
            SER_PRINTF("%s: ", olsr_ip_to_string(&buf, &tmp_list->main_addr));

            for (tmp_addr = tmp_list->aliases; tmp_addr; tmp_addr = tmp_addr->next_alias) {
                SER_PRINTF(" %s ", olsr_ip_to_string(&buf, &tmp_addr->alias));
            }

            SER_PRINTF("\n");
        }
    }
}

