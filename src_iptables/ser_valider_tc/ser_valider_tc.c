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



#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "ser_path.h"

//***********************************************************************
//***********************************************************************

static void test1(void)
{

    tc_add_node(0x11223344);
    tc_add_node(0x99887766);
    tc_add_link(0x11223344,0x99887766,55);

    tc_add_node(0xAABBCCDD);
    tc_add_link(0x11223344,0xAABBCCDD,22);

    tc_add_node(0xCCDDEEFF);
    tc_add_link(0xAABBCCDD,0xCCDDEEFF,67);
    tc_print_data(1,0);

    tc_remove_link(0x11223344,0xAABBCCDD);
    tc_print_data(1,0);

    tc_remove_node(0xAABBCCDD);
    tc_print_data(1,0);

}

//***********************************************************************
//***********************************************************************

static void test2(void)
{
    char                path_data[100];
    unsigned long       size;
    ip_v4_addr          nextHop;
    int                 index;
    struct ser_header   header;

    tc_add_node(1);

    for(index = 2; index < 51; index++)
    {
        tc_add_node(index);
        tc_add_link(index-1,index,10);
        tc_add_link(index,index-1,5);
    }

    for(index = 3; index < 51; index+=2)
    {
        tc_add_link(index-2,index,2);
    }

    tc_print_data(1,0);

    tc_remove_node(7);
    tc_remove_link(1,2);

    tc_print_data(1,0);
    pth_find_datagram_path(path_data,&size,1,2,&nextHop,&header);
}

//***********************************************************************
//***********************************************************************

static void test3(void)
{
    char                path_data[100];
    unsigned long       size;
    ip_v4_addr          nextHop;
    struct ser_header   header;


    tc_add_node(1);
    tc_add_node(2);
    tc_add_node(3);
    tc_add_node(4);
    tc_add_node(5);

    tc_add_link(1,2,1);
    tc_add_link(2,1,1);

    tc_add_link(2,3,1);
    tc_add_link(3,2,1);

    tc_add_link(3,4,1);
    tc_add_link(4,3,1);

    tc_add_link(1,4,1);
    tc_add_link(4,1,1);

    tc_add_link(1,5,1);
    tc_add_link(5,1,1);

    tc_add_link(3,5,1);
    tc_add_link(5,3,1);

    pth_find_datagram_path(path_data,&size,1,3,&nextHop,&header);
    pth_find_datagram_path(path_data,&size,1,3,&nextHop,&header);
    pth_find_datagram_path(path_data,&size,1,3,&nextHop,&header);
    pth_find_datagram_path(path_data,&size,1,3,&nextHop,&header);
    pth_find_datagram_path(path_data,&size,1,3,&nextHop,&header);
    pth_find_datagram_path(path_data,&size,1,3,&nextHop,&header);

}

//***********************************************************************
//***********************************************************************
/*
static void test4(void)
{
    char                path_data[500];
    unsigned long       size;
    ip_v4_addr          nextHop;
    ip_v4_addr          localAddr[4];
    ip_v4_addr          aliasAddr[4];
    ip_v4_addr          expectedAddr[4];    // expected addr for next hop
    int                 index1, index2;
    int                 nbNodes = 4;
    struct ser_header   header;


    localAddr[0] = 0xD10AA8C0;                    // 192.168.10.209
    localAddr[1] = 0x510AA8C0;                    // 192.168.10.81
    localAddr[2] = 0x5332A8C0;                    // 192.168.50.83
    localAddr[3] = 0x5250A8C0;                    // 192.168.80.82

    aliasAddr[0] = 0;                             //
    aliasAddr[1] = 0x5132A8C0;                    // 192.168.50.81
    aliasAddr[2] = 0x5350A8C0;                    // 192.168.80.83
    aliasAddr[3] = 0;                             //

    expectedAddr[0] = 0x510AA8C0;                 // 192.168.10.81
    expectedAddr[1] = 0x5332A8C0;                 // 192.168.50.83
    expectedAddr[2] = 0x5250A8C0;                 // 192.168.80.82
    expectedAddr[4] = 0x5250A8C0;                 // 192.168.80.82


    //---------------------------------------------
    // check path from node 0 to node : (nbNodes-1)
    //---------------------------------------------
    for(index1 = 0; index1 < nbNodes; index1++)
    {
        tc_init_data();

        //----------------
        // init local node
        //----------------
        tc_set_local_addr(localAddr[index1],0x00FFFFFF);
        if(aliasAddr[index1] != 0)
            tc_set_local_alias(aliasAddr[index1],0x00FFFFFF);

        //-------
        // init TC
        //-------

        // add node
        for(index2 = 0; index2 < nbNodes; index2++)
            tc_add_node(localAddr[index2]);

        // add links
        for(index2 = 1; index2 < nbNodes; index2++)
            tc_add_link(localAddr[index2-1],localAddr[index2],10);

        //--------------
        // validate path
        //--------------
        if(index1 ==0)
        {
            // first node
            pth_find_datagram_path(path_data,&size,localAddr[0],localAddr[nbNodes-1],&nextHop,&header);
            pth_printPath(path_data,size,0);
            printf("First hop : %lX (%s)\n", nextHop, (nextHop == expectedAddr[index1])?"ok":"KO");
        }
        else if(index1 == (nbNodes-1))
        {
            // last node
            if(pth_isEndOfPath(path_data,size) != 1)
                printf("Path failed on last hop\n");
            else
                printf("Last node reached, That's all folks !!!\n");
        }
        else
        {
            pth_checkNextHop(path_data,size,&nextHop);
            printf("Next hop  : %lX (%s)\n", nextHop, (nextHop == expectedAddr[index1])?"ok":"KO");
        }
    }
}

//***********************************************************************
//***********************************************************************

static void test5(void)
{
    char                path_data[500];
    unsigned long       size;
    ip_v4_addr          nextHop;
    ip_v4_addr          localAddr[4];
    ip_v4_addr          aliasAddr[4];
    ip_v4_addr          expectedAddr[4];    // expected addr for next hop
    int                 index1, index2;
    int                 nbNodes = 4;
    struct ser_header   header;

    localAddr[0] = 0x5250A8C0;                    // 192.168.80.82
    localAddr[1] = 0x5332A8C0;                    // 192.168.50.83
    localAddr[2] = 0x510AA8C0;                    // 192.168.10.81
    localAddr[3] = 0xD10AA8C0;                    // 192.168.10.209

    aliasAddr[0] = 0;                             //
    aliasAddr[1] = 0x5350A8C0;                    // 192.168.80.83
    aliasAddr[2] = 0x5132A8C0;                    // 192.168.50.81
    aliasAddr[3] = 0;                             //

    expectedAddr[0] = 0x5350A8C0;                 // 192.168.80.83
    expectedAddr[1] = 0x5132A8C0;                 // 192.168.50.81
    expectedAddr[2] = 0xD10AA8C0;                 // 192.168.10.209
    expectedAddr[4] = 0;                          //


    //---------------------------------------------
    // check path from node 0 to node : (nbNodes-1)
    //---------------------------------------------
    for(index1 = 0; index1 < nbNodes; index1++)
    {
        tc_init_data();

        //----------------
        // init local node
        //----------------
        tc_set_local_addr(localAddr[index1],0x00FFFFFF);
        if(aliasAddr[index1] != 0)
            tc_set_local_alias(aliasAddr[index1],0x00FFFFFF);

        //--------
        // init TC
        //--------

        // add node
        for(index2 = 0; index2 < nbNodes; index2++)
            tc_add_node(localAddr[index2]);

        // add links
        for(index2 = 1; index2 < nbNodes; index2++)
            tc_add_link(localAddr[index2-1],localAddr[index2],10);

        // set network alias 
        for(index2 = 0; index2 < nbNodes; index2++)
            tc_add_alias(localAddr[index2],aliasAddr[index2]);

        //--------------
        // validate path
        //--------------
        if(index1 ==0)
        {
            // first node
            pth_find_datagram_path(path_data,&size,localAddr[0],localAddr[nbNodes-1],&nextHop,&header);
            pth_printPath(path_data,size,0);
            printf("First hop : %lX (%s)\n", nextHop, (nextHop == expectedAddr[index1])?"ok":"KO");
        }
        else if(index1 == (nbNodes-1))
        {
            // last node
            if(pth_isEndOfPath(path_data,size) != 1)
                printf("Path failed on last hop\n");
            else
                printf("Last node reached, That's all folks !!!\n");
        }
        else
        {
            pth_checkNextHop(path_data,size,&nextHop);
            printf("Next hop  : %lX (%s)\n", nextHop, (nextHop == expectedAddr[index1])?"ok":"KO");
        }
    }
}
*/

//***********************************************************************
//***********************************************************************


int main(int argc, char *argv[])
{

    tc_init_data();

    if(0)   // to avoid warning do to useless functions
    {
        test1();
        test2();
        test3();
        //test4();
    }

    //test4();
    //test5();

    tc_free_data(1);

    return EXIT_SUCCESS;
}
