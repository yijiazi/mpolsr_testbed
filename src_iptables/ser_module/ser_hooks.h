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
#define _SER_HOOKS_

unsigned int hk_local_out_hook(unsigned int hooknum,  struct sk_buff **skb, const struct net_device *in,
                   const struct net_device *out,  int (*okfn)(struct sk_buff *));


unsigned int hk_local_in_hook(unsigned int hooknum,  struct sk_buff **skb, const struct net_device *in,
                   const struct net_device *out,  int (*okfn)(struct sk_buff *));


unsigned int hk_pre_routing_hook(unsigned int hooknum,  struct sk_buff **skb, const struct net_device *in,
                   const struct net_device *out,  int (*okfn)(struct sk_buff *));


#endif //_SER_HOOKS_

