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
#ifndef _SKB_TOOLS_H_
#define _SKB_TOOLS_H_


void skb_updateChecksumIP_UDP(struct sk_buff *skb);
void skb_updateChecksumIP(struct sk_buff *skb);

int skb_addPath(struct sk_buff *skb, char* sData, unsigned long size);
int skb_replacePath(struct sk_buff *skb, char* sData, unsigned long size, unsigned long initSize);
int skb_getPath(struct sk_buff *skb, char* sData, unsigned long* size, struct ser_header* pheader);
int skb_updatePath(struct sk_buff *skb, char* sData, unsigned long size);
int skb_removePath(struct sk_buff *skb,unsigned long size);


#endif // _SKB_TOOLS_H_

