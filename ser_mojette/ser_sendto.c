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

/************************************************************************
 *             Projet SEREADMO - N° ANR-05-RNRT-028-01
 *              Sécurité des Réseaux Ad hoc & Mojette
 *************************************************************************
 *
 * AUTEUR     : P.Lesage,A.Germonneau,S.David
 * VERSION    : 1.0
 *
 =========================================================================
 *
 * interception de la fonction 'send_to'
 *
 *************************************************************************
 * A faire :
 * -> envoyer une image (de type raw)
 * -> faire une struct pour les datagramme envoyé car ça sera plus simple
 *    pour modifier les en-têtes.(c plus propre mais pas très important)
 * -> gérer les cas où le message n'est pas d'une longueur divisible
 *    (cas des nombres premiers)
 *    sachant qu'il faudrait éviter d'envoyer des lignes de longueurs
 *    différentes car sinon le code mojette ne fonctionne plus 
 *    Le code mojette se base sur un support de type k*l..... 
 *************************************************************************
 * Synthèse du datagramme envoyé (type = projection) :
 * Octet 0 : type du datagramme (p pour une projection)
 * Octet 1 : numéro de la projection
 * Octet 2 : nombre de projections envoyées
 * Octet 3 : taille de la projection
 * Octet 4 : angle p de la projection
 * Octet 5 : angle q de la projection
 * Octet 6 : nombre de ligne (hauteur) du support (du message découpé)
 * Octet 7 : largeur du support (du message découpé)
 * A partir de l'octet 8  jusqu'à la fin : les valeurs de la projection
 *************************************************************************
 * Synthèse du datagramme envoyé (type = ligne) :
 * Octet 0 : type du datagramme (l pour une ligne)
 * Octet 1 : numéro de la ligne
 * Octet 2 : Le nombre de lignes du support
 * Octet 3 : Longueur des lignes du support
 * à partir de l'octet 4 jusqu'à la fin : les valeurs de la ligne
 *************************************************************************
 * Modifications:
 *---------------
 *
 *************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib_init.h"
// on inclut le code pour la transformée mojette
#include "mojette.h"

int sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
	//on déclare la valeur de retour -> le nb. d'octets envoyés
    int retour;
	
    if(api_sendto == NULL)
        // oups !! pb d'init de la libraire
        return -1;  // a faire : gerer la variable 'errno'
	
    if(sockDesc == s) // a faire : tenir compte du port pour pouvoir laisser passer les msg OLSR 
    {
		int i, j;

		// Pointeur pour le support
		support_t* support;

		// Pointeur sur un tableau de pointeurs(pointant vers les différentes projections) 
		projection_t** projections;
		
		//attention à avoir un message au niveau du client dont la division modulo <nb_lignes>
		// donne comme reste 0, sinon cela ne fonctionne pas
		//Taille du support
		int nb_lignes = 9 ;	//nb. de lignes
		int long_lignes = len / nb_lignes ;	//nb. de colonnes

		//Longueurs des deux types d'en-têtes (attention aux modifications)
		int long_en_tete_l = 4 ;	//en-tête ligne
		int long_en_tete_p = 8 ;	//en-tête projection

		//longueur des datagrammes de type ligne
        size_t len_ligne = len / nb_lignes + long_en_tete_l;

		// Type définit le type de datagramme :
		// 'p' : paquet projection mojette 
		// 'l' : paquet ligne correspondant à une ligne du support de projection (donc les messages).
		char type[2] = {'l','p'} ;


		//pour les tests en localhost
		int nb_lignes_env = 9; //nb. de lignes réellemment envoyées
		int nb_proj_env = 0; //nb. de projections réellemment envoyées : attention pas plus de 3 merci !

		//Les angles des projections
		int angles_p[3]={-1,0,1};//angles p (varie)
		int angles_q[3]={1,1,1};//angles q (stable)

		//On initialise le support
		support = support_new(nb_lignes, long_lignes);
		
		//On copie le message du client dans le support
		memcpy(support->pixels, msg, len);
		
		//On affiche le message original
		fprintf(stdout, "Message original :\n\n");
		for (i = 0; i < len; i ++)
			fprintf(stdout, "%c", support->pixels[i]);
		fprintf(stdout, "\n\n");
		
		//On affiche le support contenant le message original
		fprintf(stdout, "support contenant le message original :\n\n");
		for (i = 0; i < nb_lignes; i ++)
		{
			for (j = 0; j < long_lignes; j ++)
				fprintf(stdout, "%c", support->pixels[i * support->k + j]);
			fprintf(stdout, "\n");
		}
		fprintf(stdout, "\n");

		// Remplissage et envoi des datagrammes de type ligne 
		for (i = 0 ; i < nb_lignes_env ; i++) {
			
			// On alloue la mémoire nécessaire pour stocker le datagramme de type ligne
			char* datagram = (char*) malloc(len_ligne * sizeof(char));

			// Le type de datagramme
			datagram[0] = type[0] ;
			
			// Le numéro de la ligne
			datagram[1] = i ;

			// Le nombre de lignes du support
			datagram[2] = nb_lignes;
					
			// Longueur des lignes du support
			datagram[3] = long_lignes;

			// Valeurs contenues dans la ligne
			for(j = 0; (j < long_lignes) && (j + long_en_tete_l) < (long_en_tete_l + long_lignes); j++)
			{
				datagram[j+long_en_tete_l] = support->pixels[(i * long_lignes) + j] ;
			}
	
			//envoi du datagramme de type ligne
			retour = api_sendto(s, datagram, len_ligne, flags, to, tolen);

			// Impression du datagramme envoyé sur le terminal
			fprintf(stdout, "---------------------------------------\n");
			fprintf(stdout, "Envoi d'un datagramme de type ligne\n\n");
			fprintf(stdout, "Type : %c \n", type[0]);
			fprintf(stdout, "Numero de la ligne : %d \n", i);
			fprintf(stdout, "Nombre de lignes : %d \n", nb_lignes);
			fprintf(stdout, "Longueur de la ligne : %d \n", long_lignes);
			fprintf(stdout, "Ligne envoyee : ");
			for(j = 0; (j < long_lignes) && (j + long_en_tete_l) < (long_en_tete_l + long_lignes); j++) {
				fprintf(stdout, "%c", datagram[j+long_en_tete_l]);
			}
			fprintf(stdout, "\n");
			fprintf(stdout, "Longueur total du datagramme envoye : %d octets\n", retour);
			fprintf(stdout, "---------------------------------------\n");

			//libération de la mémoire allouée
			free(datagram);
		}
			
		// Allocation de la mémoire pour les projections
		projections = malloc(nb_proj_env * sizeof(projection_t*));

		//On initialise les projections suivant les angles choisis et la taille du support
		for (i = 0 ; i < nb_proj_env ; i++)
		{
			projections[i] = projection_new(angles_p[i], angles_q[i], nb_lignes, long_lignes);
		}
	 	
		// Calcul des projections 
		forward(support, projections, nb_proj_env);		

		// Remplissage et envoi des datagrammes de type projection 		
		for (i = 0 ; i < nb_proj_env ; i++) {
			
			// On alloue la mémoire nécessaire pour stocker le datagramme de type ligne
			char* datagram = (char*) malloc(long_en_tete_p +projections[i]->size);

			// Le type de datagramme
			datagram[0] = type[1] ;
			
			// Le numéro de la projection
			datagram[1] = i ;
			
			//le nombre de projections envoyées
			datagram[2] = nb_proj_env; 
			
			//taille de la projection
			datagram[3] = projections[i]->size;

			//angle P de la projection			
			datagram[4] = projections[i]->p;

			//angle Q de la projection			
			datagram[5] = projections[i]->q;

			//Nombre de lignes du support				
			datagram[6] = nb_lignes;
			
			//Longueurs des lignes du support	
			datagram[7] = long_lignes;
			
			// Valeurs contenues dans la projection
			for(j = 0; j< projections[i]->size && j+long_en_tete_p < (long_en_tete_p +projections[i]->size); j++)
			{
				datagram[j+long_en_tete_p] = projections[i]->bins[j] ;
			}

			//envoi du datagramme de type projection
			retour = api_sendto(s,datagram, (long_en_tete_p +projections[i]->size),flags,to, tolen);
			
			// Impression du datagramme envoyé sur le terminal
			fprintf(stdout, "---------------------------------------\n");
			fprintf(stdout, "Envoi d'un datagramme de type projection\n\n");			
			fprintf(stdout, "Type : %c \n", type[1]);
			fprintf(stdout, "Numero de la projection : %d \n", i);
			fprintf(stdout, "Nombre de projections envoyees : %d \n", nb_proj_env);
			fprintf(stdout, "Longueur de la projection : %d \n", projections[i]->size);
			fprintf(stdout, "Angle P de la projection : %d \n", projections[i]->p);
			fprintf(stdout, "Angle Q de la projection : %d \n", projections[i]->q);
			fprintf(stdout, "Nombre de lignes : %d \n", nb_lignes);
			fprintf(stdout, "Longueur des lignes : %d \n", long_lignes);
			fprintf(stdout, "Projection envoyee : ");
			for(j = 0; j< projections[i]->size && j+long_en_tete_p < (long_en_tete_p +projections[i]->size); j++)
			{
				fprintf(stdout, "%c", datagram[j+long_en_tete_p]);
			}
			fprintf(stdout, "\n");
			fprintf(stdout, "Longueur total du datagramme envoye : %d octets\n", retour);
			fprintf(stdout, "---------------------------------------\n");

			//libération de la mémoire allouée
			free(datagram);
		}

		return retour;
    }
    else
    {
        // pas de modification des donnees => appel direct de la fonction d'origine
        retour = api_sendto(s,msg, len,flags,to, tolen);
    }
    // renvoie du code de retour inital
    return retour;
}
