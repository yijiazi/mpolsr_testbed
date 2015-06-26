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
*****************************************************************

*
* Module main file.
* Provide : - module init (set iptables hooks )
*           - module uninit (remove iptables hooks)
*
*************************************************************************
* Changes :
* --------
*
*************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include "/usr/include/linux/netfilter_ipv4.h"

#include "common.h"
#include "ser_hooks.h"
#include "ser_path.h"
#include "ser_device.h"



MODULE_DESCRIPTION("Sereadmo Iptables Module");
MODULE_AUTHOR("Pascal Lesage (pascal.lesage@keosys.com)");
MODULE_LICENSE("Dual");

module_param_named(act_hook,g_act_hook,int,S_IRUGO);
module_param_named(act_device,g_act_device,int,S_IRUGO);
module_param_named(act_recovery,g_act_recovery,int,S_IRUGO);

module_param_named(dbg_hook,g_dbg_hook,int,S_IRUGO);
module_param_named(dbg_device,g_dbg_device,int,S_IRUGO);
module_param_named(dbg_module,g_dbg_module,int,S_IRUGO);
module_param_named(dbg_djk,g_dbg_djk,int,S_IRUGO);

module_param_named(nb_path,g_conf_path,int,S_IRUGO);
module_param_named(coef_fe,g_conf_fe,int,S_IRUGO);
module_param_named(coef_fp,g_conf_fp,int,S_IRUGO);
module_param_named(udp_port,g_conf_udp_port,int,S_IRUGO);



//***********************************************************************
//***********************************************************************


static struct nf_hook_ops g_ser_hook_ops[] = {
    {
        .hook       = hk_local_out_hook,
        .owner      = THIS_MODULE,
        .pf         = PF_INET,
        .hooknum    = NF_IP_LOCAL_OUT,
    },
    {
        .hook       = hk_local_in_hook,
        .owner      = THIS_MODULE,
        .pf         = PF_INET,
        .hooknum    = NF_IP_LOCAL_IN,
    },
    {
        .hook       = hk_pre_routing_hook,
        .owner      = THIS_MODULE,
        .pf         = PF_INET,
        .hooknum    = NF_IP_PRE_ROUTING,
    },

};

//***********************************************************************
//***********************************************************************



static int ser_iptables_init_module(void)
{
    int i = 0;
    int ret = 0;

    DBG_MOD(1,"Module ser_iptables init\n" );

    if((ret = tc_init_data()) <0)
        // failed to init TC data
        goto cleanup_hooks;


    if(g_act_device != 0)
    {
        if((ret = dev_init_device()) <0)
            // failed to init char device
            goto cleanup_hooks;
    }

    if(g_act_hook != 0)
    {
        // add iptables hooks
        for(i=0; i < ARRAY_SIZE(g_ser_hook_ops); i++){
            if((ret = nf_register_hook(&g_ser_hook_ops[i])) <0)
            {
                MSG_FAILED("Failed to set netfilter hook\n" );
                goto cleanup_hooks; 
            }
        }
    }
    return ret;

cleanup_hooks:

    MSG_FAILED("Failed to load sereadmo - netfilter module\n" );

    // remove hooks after init failed
    if(g_act_hook != 0)
    {
        while(--i > 0)
            nf_unregister_hook(&g_ser_hook_ops[i]);
    }

    return ret;
}


//***********************************************************************
//***********************************************************************


static void ser_iptables_exit_module(void)
{
    int i;

    // remove hooks
    if(g_act_hook != 0)
    {
        for(i=0; i < ARRAY_SIZE(g_ser_hook_ops); i++)
            nf_unregister_hook(&g_ser_hook_ops[i]);
    }

    if(g_act_device != 0)
        dev_cleanup_device();

    tc_free_data(1);

    DBG_MOD(1,"Module ser_iptables exit\n" );
}


//***********************************************************************
//***********************************************************************


module_init(ser_iptables_init_module);
module_exit(ser_iptables_exit_module);
