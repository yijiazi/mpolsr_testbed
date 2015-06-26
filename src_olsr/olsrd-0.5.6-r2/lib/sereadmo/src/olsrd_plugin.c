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
******************************************************************

*
=========================================================================
*
* Implement dynamique library startup and cleanup (for Linux and OLSRD)
*
*************************************************************************
* Changes :
* --------
*
*************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "olsr.h"
#include "olsrd_plugin.h"
#include "ser_global.h"
#include "ser_data.h"
#include "ser_device.h"

#define PLUGIN_NAME                 "OLSRD SEREADMO plugin"
#define PLUGIN_VERSION              "0.1"
#define PLUGIN_AUTHOR               "Pascal Lesage" // me !!!
#define MOD_DESC PLUGIN_NAME        " " PLUGIN_VERSION " by " PLUGIN_AUTHOR
#define PLUGIN_INTERFACE_VERSION    5   // olsrd compatibility version


//****************************************************************************
//****************************************************************************

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

//------------------------------------------------------------------
// define and implement Linux dynamique library startup and cleanup 
// (ie. not OLSR functions)
//------------------------------------------------------------------
// (def) called by Linux during library startup
static void my_init(void) __attribute__((constructor));

// (def) called by Linux during library cleanup 
static void my_fini(void) __attribute__((destructor));

//------------------------------------------------------------------

// (impl) called by Linux during library startup
static void my_init(void)
{
    // initialise device name with default vavlue
    strcpy(g_ser_device_name,DEFAULT_SER_DEVICE);

    // OLSR initialise the plugin by a call to 
    // 'olsrd_plugin_init' function after library startup
}


//------------------------------------------------------------------


// (impl) called by Linux during library cleanup 
static void my_fini(void)
{
    // call (this) plugin cleanup
    olsrd_plugin_exit();
}



//****************************************************************************
//****************************************************************************



//-------------------------------------------------------
// define and implement specifics OLSRD plugins functions
//-------------------------------------------------------

// retrieve interface version for compatibility checking
int  olsrd_plugin_interface_version(void)
{
    return PLUGIN_INTERFACE_VERSION;
}



//-----------------------------------------------------


// define plugin paramameters
// (read from OLSRD conf file)
static const struct olsrd_plugin_parameters g_plugin_parameters[] = {
    { .name = "device",   
      .set_plugin_parameter = &set_plugin_string,
      .data = &g_ser_device_name,  
      .addon = {sizeof(g_ser_device_name)}
    },
};


//-----------------------------------------------------


// called by OLSR to retrieve parameters definition
void olsrd_get_plugin_parameters(const struct olsrd_plugin_parameters **params, int *size)
{
    *params = g_plugin_parameters;
    *size = sizeof(g_plugin_parameters)/sizeof(*g_plugin_parameters);
}


//-----------------------------------------------------


// called by OLSR after library startup to initialise the plugin
int olsrd_plugin_init(void)
{
    int ret;

    SER_PRINTF("olsrd_plugin_init\n");
    
    // initialise connection to SEREADMO-netfilter char device
    ret = dev_device_init();
    if(ret < 0)
        return ret;

    // add callback on OLSRD TC modification
    register_pcf(&olsrd_pcf_event);

    return 1;
}


//-----------------------------------------------------


// called to cleanup plugin
void olsrd_plugin_exit(void)
{
    // close connection to SEREADMO-netfilter char device
    dev_device_clean();
}

