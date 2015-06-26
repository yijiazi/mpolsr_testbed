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

#ifndef _SER_DEVICE
#define _SER_DEVICE

#include "olsr_types.h"


#define DEFAULT_SER_DEVICE "/dev/sereadmo"
//#define DEFAULT_SER_DEVICE "/foo/bar"


extern char     g_ser_device_name[50];


void dev_send_init(void);
void dev_send(void);

void dev_add_node(union olsr_ip_addr* addr);
void dev_rmv_node(union olsr_ip_addr* addr);

void dev_rmv_link(union olsr_ip_addr* src_addr, union olsr_ip_addr* dst_addr);
void dev_add_link(union olsr_ip_addr* src_addr, union olsr_ip_addr* dst_addr, int qlt);

void dev_add_alias(union olsr_ip_addr* addr, union olsr_ip_addr* mask);
void dev_rmv_alias(union olsr_ip_addr* addr, union olsr_ip_addr* mask);

int dev_device_init(void);
void dev_device_clean(void);

#endif // _SER_DEVICE
