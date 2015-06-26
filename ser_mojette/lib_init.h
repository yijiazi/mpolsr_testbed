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
*******************************************************************/

#include <sys/socket.h>
#include <sys/types.h>


// definition des signatures des fonctions Socket
//-----------------------------------------------
typedef int (*ser_socket)(int domain, int type, int protocol);
typedef int (*ser_sendto)(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);


// definition des handles vers les API Socket d'origne
//----------------------------------------------------
extern ser_sendto api_sendto;
extern ser_socket api_socket;



// liste des descripteurs de socket
//---------------------------------
extern int sockDesc; // a faire : remplacer par une liste

