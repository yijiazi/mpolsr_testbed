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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include <netinet/in.h>
#include "rawsocket.h"

namespace sereadmo {
namespace net {

/*
 * rawsock_init():
 *	prepare the raw socket and return the handle (file descriptor)
 */
RawSocket::RawSocket()
{
  char on = 1;
  int r;
  int onr = 1;

	/* raw socket */
	if ((sock_ = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("socket(AF_INET, SOCK_RAW, IPPROTO_RAW)");
		return;
	}
	r = setsockopt(sock_, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
	if (r < 0) {
		close();
		perror("setsockopt(IPPROTO_IP)");
        return;
	}

	/* set SO_REUSEADDR so that more than one process can use the socket */
	if(setsockopt(sock_,SOL_SOCKET, SO_REUSEADDR, &onr, sizeof(onr)) < 0)
	{
		close();
		perror("rawsock_init() : setsockopt");
	}
}

RawSocket::~RawSocket()
{
  close();
}


/*
 * rawsock_out(buf, len, flags):
 *	write the packet from buf into fd, return bytes actually written
 */
int RawSocket::sendTo(ip_v4_addr daddr, unsigned char *buf, int len, int flags)
{
  int r;
  struct sockaddr_in sin;

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr=daddr;
  sin.sin_port = 0;

  r = sendto(sock_, (void *)buf, len, flags,
          (struct sockaddr *)&sin, sizeof(sin));

#ifdef DEBUG
	{ int i;
		fprintf(stderr, "OUT: (%d) [ ", len);
		for (i = 0; i < 20; i++)
			fprintf(stderr, "%02x ", buf[i]);
		fprintf(stderr, "] = %d\n", r);
	}
#endif

	if (r < 0)
		perror("RawSocket::write()");
	
	return r;
}

bool RawSocket::close() 
{ 
   ::close(sock_); 
   return true; 
} 

}
}