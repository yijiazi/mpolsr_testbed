/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2004, Andreas T�nnesen(andreto@olsr.org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 *
 * * Redistributions of source code must retain the above copyright 
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in 
 *   the documentation and/or other materials provided with the 
 *   distribution.
 * * Neither the name of olsr.org, olsrd nor the names of its 
 *   contributors may be used to endorse or promote products derived 
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Visit http://www.olsr.org for more information.
 *
 * If you find this software useful feel free to make a donation
 * to the project. For more information see the website or contact
 * the copyright holders.
 *
 */

/*
 *
 *IPC - interprocess communication
 *for the OLSRD - GUI front-end
 *
 */

#ifndef _OLSR_IPC
#define _OLSR_IPC

#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>

#include "defs.h"

#define IPC_PORT 1212
#define IPC_PACK_SIZE 44 /* Size of the IPC_ROUTE packet */
#define	ROUTE_IPC 11    /* IPC to front-end telling of route changes */
#define NET_IPC 12      /* IPC to front end net-info */

/*
 *IPC message sent to the front-end
 *at every route update. Both delete
 *and add
 */

struct ipcmsg 
{
  olsr_u8_t          msgtype;
  olsr_u16_t         size;
  olsr_u8_t          metric;
  olsr_u8_t          add;
  union olsr_ip_addr target_addr;
  union olsr_ip_addr gateway_addr;
  char               device[4];
};


struct ipc_net_msg
{
  olsr_u8_t            msgtype;
  olsr_u16_t           size;
  olsr_u8_t            mids; /* No. of extra interfaces */
  olsr_u8_t            hnas; /* No. of HNA nets */
  olsr_u8_t            unused1;
  olsr_u16_t           hello_int;
  olsr_u16_t           hello_lan_int;
  olsr_u16_t           tc_int;
  olsr_u16_t           neigh_hold;
  olsr_u16_t           topology_hold;
  olsr_u8_t            ipv6;
  union olsr_ip_addr   main_addr;
};


olsr_bool
ipc_check_allowed_ip(const union olsr_ip_addr *);

void
ipc_accept(int);

void
frontend_msgparser(union olsr_message *, struct interface *, union olsr_ip_addr *);

int
ipc_route_send_rtentry(const union olsr_ip_addr *, const union olsr_ip_addr *, int, int, const char *);

#endif
