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
*************************************************************************
* Used and modified 2015
*   by      Benjamin Mollé (engineering student, Polytech Nantes, University of Nantes, France) benjamin.molle@gmail.com
*           Denis Souron (engineering student, Polytech Nantes, Université of Nantes, France) denis.souron@laposte.net
*************************************************************************/
#ifndef _SER_PATH_H
#define _SER_PATH_H

#define MPOLSR_VERSION 1
#define IPPROTO_MPOLSR 99


// header of SEREAMO information in IP datagram
struct ser_header
{
    unsigned char   version;
    unsigned char   protocol;
    unsigned short  empty;      // useless field
    unsigned short  hops;
    unsigned short  cur;
    unsigned long   first_node;
};


// complete SEREADMO information in IP datagram
// = header + main node IP of all hops
struct ser_data
{
    struct ser_header sh;
    unsigned long* addr_tab;
};

#ifdef __cplusplus
extern "C" {
#endif

//---------------
// Path functions
//---------------

int pth_find_datagram_path(char* path, unsigned long *pSize, ip_v4_addr srcAddr, 
                            ip_v4_addr dstAddr, ip_v4_addr* nextHop, struct ser_header* pheader);
int pth_isEndOfPath(char* path);
int pth_checkCurrentAddr(ip_v4_addr curr_addr);
int pth_checkNextHop(ip_v4_addr* nextHop);
int pth_getLastNode(char* path, unsigned long size, ip_v4_addr* lastNode);

// for debug
int pth_printPath(char* path, unsigned long size, int dbg_level);



//-------------
// TC functions
//-------------

int  tc_init_data(void);
void tc_free_data(int all);

void tc_add_node(ip_v4_addr addr_ip);
void tc_remove_node(ip_v4_addr addr_ip);

void tc_add_link(ip_v4_addr addr_src, ip_v4_addr addr_dst, int cost);
void tc_remove_link(ip_v4_addr addr_src, ip_v4_addr addr_dst);

void tc_add_alias(ip_v4_addr addr_ip, ip_v4_addr addr_alias);
void tc_remove_alias(ip_v4_addr addr_ip, ip_v4_addr addr_alias);

void tc_set_local_addr(ip_v4_addr addr_ip, ip_v4_addr mask);
void tc_set_local_alias(ip_v4_addr addr_ip, ip_v4_addr mask);

struct olsr_node* tc_get_node(ip_v4_addr addr_ip);

//---------------
// Test functions
//---------------
void tc_print_data(int withLock, int dbg_level);

#ifdef __cplusplus
}
#endif

#endif //_SER_PATH_H



