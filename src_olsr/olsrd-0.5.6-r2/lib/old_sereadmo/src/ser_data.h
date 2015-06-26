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
#ifndef _SER_DATA
#define _SER_DATA

#include "olsrd_plugin.h"
#include "plugin_util.h"


int olsrd_pcf_event(int changes_neighborhood, int changes_topology, int changes_hna);


//Exported library functions needed by OLSRD to use the plugin
int olsrd_plugin_interface_version(void);
int olsrd_plugin_init(void);
void olsrd_plugin_exit(void);
void olsrd_get_plugin_parameters(const struct olsrd_plugin_parameters **params, int *size);

#endif
