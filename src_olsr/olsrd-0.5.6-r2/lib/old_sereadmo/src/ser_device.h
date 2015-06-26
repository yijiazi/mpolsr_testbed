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
#ifndef _SER_DEVICE
#define _SER_DEVICE

#include "olsr_types.h"


#define DEFAULT_SER_DEVICE "/dev/sereadmo"


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
