/************************************************************************
*             Projet SEREADMO - N° ANR-05-RNRT-028-01
*              Sécurité des Réseaux Ad hoc & Mojette
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
#include "ser_data.h"
#include "ser_device.h"
#include "ser_tc.h"
#include "ser_global.h"

//  Scheduled event
int olsrd_pcf_event(int changes_neighborhood, int changes_topology, int changes_hna)
{
    int useless;

    useless = changes_neighborhood || changes_hna || changes_topology; // useless code to avoid warning with gcc

    tc_check_data();

    dev_send();

    return 0;
}


