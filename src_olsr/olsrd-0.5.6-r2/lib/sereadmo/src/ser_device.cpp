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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
extern "C" {
#include "ipcalc.h"
#include "defs.h"
#include "ser_device.h"
#include "ser_tc.h"
#include "ser_global.h"
}

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "boost/thread.hpp"

#include "olsreventserver.h"

//****************************************************************************
//****************************************************************************


#define DEV_CMD_RESET_TC        1
#define DEV_CMD_ADD_NODE        2
#define DEV_CMD_REMOVE_NODE     3
#define DEV_CMD_ADD_LINK        4
#define DEV_CMD_REMOVE_LINK     5
#define DEV_CMD_ADD_ALIAS       6
#define DEV_CMD_REMOVE_ALIAS    7
#define DEV_CMD_SET_LOCAL_ADDR  8
#define DEV_CMD_SET_ALIAS_ADDR  9





// token sent to sereadmo-netfilter mode
struct dev_tc_token
{
    long            type;
    unsigned long   addr_1;
    unsigned long   addr_2;
    int             value;
};



struct dev_cmd
{
    struct dev_tc_token token;
    struct dev_cmd* next;
};

//****************************************************************************
//****************************************************************************



char                g_ser_device_name[50];
FILE*               g_ser_file_device = NULL;
struct dev_cmd*     g_cmd_lst = NULL;
struct dev_cmd*     g_cmd_lst_end = NULL;

static boost::asio::io_service io_service;
static OlsrEventServerPtr g_olsrEventServer;

//****************************************************************************
//****************************************************************************


static void dev_add_cmd(int type, union olsr_ip_addr* src_addr, union olsr_ip_addr* dst_addr, int value)
{
    //struct dev_cmd* cmd;
    OlsrEventMessage msg;

    SER_PRINTF("add cmd : %d - %X - %X - %d",type,
            (src_addr == NULL)?0:src_addr->v4.s_addr,
            (dst_addr == NULL)?0:dst_addr->v4.s_addr, value);

    //cmd = (struct dev_cmd*)malloc(sizeof(struct dev_cmd));
    //memset(cmd,0,sizeof(struct dev_cmd));

    msg.type = type;
    //cmd->token.type = type;
    if(src_addr != NULL)
        msg.addr_1 = src_addr->v4.s_addr;

    if(dst_addr != NULL)
        msg.addr_2 = dst_addr->v4.s_addr;

    msg.value = value;

    g_olsrEventServer->addMessage(msg);
    
/*    if(g_cmd_lst == NULL)
        g_cmd_lst = cmd;

    if(g_cmd_lst_end == NULL)
    {
        g_cmd_lst_end = cmd;
    }
    else
    {
        g_cmd_lst_end->next = cmd;
        g_cmd_lst_end = cmd;
    }*/
}


//****************************************************************************
//****************************************************************************


void dev_send_init(void)
{
    union olsr_ip_addr* addr, mask;
    struct interface* ifs;

    //------------------------------
    // reset sereadmo iptable module
    //------------------------------
    dev_add_cmd(DEV_CMD_RESET_TC,NULL,NULL,0);


    //------------------
    // send local config
    //------------------

    //main addr (ie. addr used by olsrd in links for local node)
    addr = &olsr_cnf->main_addr;

    // retreive IP mask for the main addr
    for(ifs = ifnet; ifs != NULL; ifs = ifs->int_next)
    {
        if(ipequal(addr,&ifs->ip_addr) != 0)
        {
            mask.v4.s_addr = ifs->int_netmask.sin_addr.s_addr;
            break;
        }
    }

    dev_add_cmd(DEV_CMD_SET_LOCAL_ADDR,addr,&mask,0);

    //----------------
    // send addr alias
    //----------------
    for(ifs = ifnet; ifs != NULL; ifs = ifs->int_next)
    {
        if(ipequal(addr,&ifs->ip_addr) == 0)
        {
            mask.v4.s_addr = ifs->int_netmask.sin_addr.s_addr;
            dev_add_cmd(DEV_CMD_SET_ALIAS_ADDR,&ifs->ip_addr,&mask,0);
        }
    }

    dev_send();
}


//****************************************************************************
//****************************************************************************


void dev_rmv_node(union olsr_ip_addr* addr)
{
    dev_add_cmd(DEV_CMD_REMOVE_NODE,addr,NULL,0);
}


//****************************************************************************
//****************************************************************************


void dev_rmv_link(union olsr_ip_addr* src_addr, union olsr_ip_addr* dst_addr)
{
    dev_add_cmd(DEV_CMD_REMOVE_LINK,src_addr,dst_addr,0);
}


//****************************************************************************
//****************************************************************************


void dev_add_node(union olsr_ip_addr* addr)
{
    dev_add_cmd(DEV_CMD_ADD_NODE,addr,NULL,0);
}


//****************************************************************************
//****************************************************************************


void dev_add_link(union olsr_ip_addr* src_addr, union olsr_ip_addr* dst_addr, int qlt)
{
    dev_add_cmd(DEV_CMD_ADD_LINK,src_addr,dst_addr,qlt);
}


//****************************************************************************
//****************************************************************************


void dev_add_alias(union olsr_ip_addr* addr, union olsr_ip_addr* alias)
{
    dev_add_cmd(DEV_CMD_ADD_ALIAS,addr,alias,0);
}

//****************************************************************************
//****************************************************************************


void dev_rmv_alias(union olsr_ip_addr* addr, union olsr_ip_addr* alias)
{
    dev_add_cmd(DEV_CMD_REMOVE_ALIAS,addr,alias,0);
}


//****************************************************************************
//****************************************************************************


void dev_send(void)
{
    g_olsrEventServer->notify();
/*    struct dev_cmd* cmd, *cmd_tmp;
    int size;

  //  SER_PRINTF("--- send cmd ---\n");


    if (g_olsrEventServer == NULL)
      return;
//    if(g_ser_file_device == NULL)
//        return;


    size = sizeof(struct dev_tc_token);
    cmd = g_cmd_lst;
    g_cmd_lst = NULL;
    g_cmd_lst_end = NULL;
    while(cmd)
    {
     //   SER_PRINTF("    send cmd : %ld - %lX - %lX - %d\n",cmd->token.type,
       //         cmd->token.addr_1,cmd->token.addr_2,cmd->token.value);


        if(fwrite(&cmd->token,size,1,g_ser_file_device) != 1)
        {
            printf("Failed to send cmd to SEREADMO char device\n");
            //TODO : reset TC data
        }

        fflush(g_ser_file_device);

        cmd_tmp = cmd;
        cmd = cmd->next;
        free(cmd_tmp);
    }*/
}

//****************************************************************************
//****************************************************************************

int dev_device_init(void)
{
    // initialise memory
    tc_init_data();
    
    SER_PRINTF("dev_device_init\n");

/*
    // connect to SEREADMO char device
    g_ser_file_device = fopen(g_ser_device_name,"r+b");

    if(g_ser_file_device == NULL)
        // failed to open device
        return -1;
*/
    g_olsrEventServer.reset(new OlsrEventServer(io_service, 7171));
  
    // Lancement du service principal dans un thread. Il continue tant qu'il reste des
    // opérations asynchrones en cours (il ne s'arrête donc jamais)
    boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));
    //io_service.run();

    // send init cmd to iptables device
    dev_send_init();

    return 0;
}

//****************************************************************************
//****************************************************************************


void dev_device_clean(void)
{

    if(g_ser_file_device ==  NULL)
        return;

    // send init cmd to remove all path / nodes / links / ...
    dev_send_init();

    fclose(g_ser_file_device);
}


