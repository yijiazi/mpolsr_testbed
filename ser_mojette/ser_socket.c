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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib_init.h"



int socket(int domain, int type, int protocol)
{
    int retour; // code de retour

    if(api_socket == NULL)
        // oups !! pb d'init de la libraire
        return -1;  // a faire : gerer la variable 'errno'


    // appel de la fonction d'origine
    retour = api_socket(domain, type, protocol);
    if(retour == -1)
        return retour; // echec

    // memorisation du descripteur de la socket
    if(type == SOCK_DGRAM)
        sockDesc = retour;

    // revoie du code de retour inital
    return retour;
}

