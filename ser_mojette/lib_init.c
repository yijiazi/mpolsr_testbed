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

#define _GNU_SOURCE /* pour l'utilisation de RTLD_NEXT */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include "lib_init.h"


// pour les tests
// LD_PRELOAD=/sereadmo/src/ser_mojette/ser_mojette.so /sereadmo/src/client/client 127.0.0.1 104




// definition des methodes d'init et de liberation des reources de la librairie
//-----------------------------------------------------------------------------
static void __attribute__ ((constructor))   lib_init(void);
static void __attribute__ ((destructor))    lib_uninit(void);

// definition des handles vers les API Socket d'origne
//----------------------------------------------------
ser_sendto api_sendto = NULL;
ser_socket api_socket = NULL;


// liste des descripteurs de socket
//---------------------------------
int sockDesc = 0;


/* initialisation de la librairie */
static void lib_init(void) 
{
    // recupration des fonctions API socket d'origine
    api_sendto  = (ser_sendto) dlsym(RTLD_NEXT,"sendto");
    api_socket  = (ser_socket) dlsym(RTLD_NEXT,"socket");

    printf("Library ready. \n");
}


/* liberation des resources */
static void lib_uninit(void) 
{
  printf("Library removed. \n");

}


