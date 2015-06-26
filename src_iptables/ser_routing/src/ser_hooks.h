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
#ifndef _SER_HOOKS_

extern "C" {
#include "/usr/local/include/libnetfilter_queue/libnetfilter_queue.h"
}
#define _SER_HOOKS_
namespace sereadmo {
namespace nfhooks {

unsigned int hk_local_out_hook(struct nfq_q_handle *qh, struct nfq_data *nfa);

unsigned int hk_local_in_hook(struct nfq_q_handle *qh, struct nfq_data *nfa);

unsigned int hk_pre_routing_hook(struct nfq_q_handle *qh, struct nfq_data *nfa);

}
}
#endif //_SER_HOOKS_

