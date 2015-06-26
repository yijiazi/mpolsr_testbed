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
* Creates a read-only char device
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
#ifndef _SER_DEVICE_
#define _SER_DEVICE_


#define DEV_CMD_RESET_TC        1
#define DEV_CMD_ADD_NODE        2
#define DEV_CMD_REMOVE_NODE     3
#define DEV_CMD_ADD_LINK        4
#define DEV_CMD_REMOVE_LINK     5
#define DEV_CMD_ADD_ALIAS       6
#define DEV_CMD_REMOVE_ALIAS    7
#define DEV_CMD_SET_LOCAL_ADDR  8
#define DEV_CMD_SET_ALIAS_ADDR  9


#define DEVICE_NAME             "seriptables" /* Dev name as it appears in /proc/devices   */


struct dev_tc_token
{
    long            type;
    ip_v4_addr      addr_1;
    ip_v4_addr      addr_2;
    int             value;
};


int dev_init_device(void);
void dev_cleanup_device(void);


#endif // _SER_DEVICE_

