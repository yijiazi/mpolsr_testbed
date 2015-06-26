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
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <time.h>
#include "common.h"

//---------------------------------------------------
// module parameters (in /etc/modprob.d/ser_iptables)
//---------------------------------------------------

int g_act_hook = 1;
int g_act_device = 1;
int g_act_recovery = 1;

int g_dbg_hook = 10;
int g_dbg_device = 10;
int g_dbg_module = 10;
int g_dbg_djk = 10;

int g_conf_path = 3;
int g_conf_fe = 2;
int g_conf_fp = 2;
int g_conf_udp_port = 104;


void kprintf(const char *format, ...)
{
    time_t      curDate;
    struct tm   stCurDate;
    va_list     args;

    va_start(args,format);

    // recup de l'heure courante
    time(&curDate);
    localtime_r(&curDate,&stCurDate);

    // ajout de la date dans les logs
    printf("%02d%02d%04d %02d:%02d:%02d : ",stCurDate.tm_mday,stCurDate.tm_mon+1,
            stCurDate.tm_year+1900, stCurDate.tm_hour,stCurDate.tm_min, stCurDate.tm_sec);

    // ajout du message utilisateur
    vprintf(format,args);
    printf("\n");

    fflush(stdout);

     va_end(args);
}

char* ipToString(unsigned long ipAddr)
{
  static char tabStr[3][16];
  static int i=-1;
  i = (i+1) % 3;
  return strcpy((char*)tabStr[i], inet_ntoa(*(struct in_addr*)&ipAddr));
}
