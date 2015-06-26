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
/*
*
* Noeud récepteur : réception de plusieurs datagrammes
* avec reconstruction mojette (systématique ou non)
*
*************************************************************************
* A faire :
* recevoir une image et la reconstruire
*
*************************************************************************
* Hypothèse sur le datagramme recu (type = projection) :
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
* Hypothèse sur le datagramme recu (type = ligne) :
* Octet 0 : type du datagramme (l pour une ligne)
* Octet 1 : numéro de la ligne
* Octet 2 : Le nombre de lignes du support
* Octet 3 : Longueur des lignes du support
* à partir de l'octet 4 jusqu'à la fin : les valeurs de la ligne
*************************************************************************
* Pour lancer le programme
* ./serveur num_port
*************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mojette.h"
#include <unistd.h>

void error(char *msg)
{
	perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
	//déclarations pour la socket
	int sock, length, fromlen, n;
	struct sockaddr_in server;
	struct sockaddr_in from;

	// Pour nos boucles
	int i,j;

	// longueur maximal des datagrammes reçus
   	char buf[1024]; //attention peut-être plus?
	
	//Si on ne saisi pas de port
	if (argc < 2) {
		fprintf(stderr, "Erreur, mettez le numéro du port en argument\n");
		exit(0);
	}

	//initialisation de la socket
	sock=socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) error("Opening socket");
	length = sizeof(server);
	bzero(&server,length);
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=htons(atoi(argv[1]));

	if (bind(sock,(struct sockaddr *)&server,length)<0) 
	error("binding");
	fromlen = sizeof(struct sockaddr_in);

	// Pointeur sur un tableau de pointeurs(pointant vers les différentes projections) 
	projection_t** projections_rec;
	
	// Pointeur sur un tableau de pointeurs(pointant vers les différentes lignes) 
	line_t** lignes_rec;

	// Pointeur sur le support
	support_t* support_rec;

	// initialisation de nb_projections
	int nb_projections;
	int l;//nb. de lignes
	int k;//longueur des lignes

	//nombre de projections recues -> au début 0
	int nb_projec_rec = 0;

	//nombre de datagrammes (n'importe quels types) reçus -> au début 0
	int nb_data_rec = 0;

	//nombre de lignes reçues -> au début 0
	int nb_ligne_rec = 0;

	//on écoute en continu sur le port saisi
	while (1) {

		//on recoit un datagramme
		n = recvfrom(sock,buf,sizeof(buf),0,(struct sockaddr *)&from,&fromlen);
			
		// Timer qui se déclenche lorsqu'on reçoit le premier datagramme
		time_t time_1;
		time_t time_2;
		double total;
		if(nb_data_rec == 0){
			time_1=time(&time_1);
		}

		//si il y a un problème à la réception
		if (n < 0) error("recvfrom");

		//On regarde le type du datagramme reçu
		char type_data = buf[0];

		//si le datagramme est une ligne original du support
		if(type_data == 'l'){
					
			// Le nombre de lignes du support original
			l = buf[2];
		
			// Si c'est la première ligne reçue alors on alloue la mémoire nécessaire
			// pour stocker toutes les lignes
			if ( nb_ligne_rec == 0){
				lignes_rec = malloc(l * sizeof(line_t*));
			}

			//on incrémente le nombre de lignes reçues
			nb_ligne_rec ++;

			//On alloue la mémoire pour stocker la ligne
			line_t* ligne_1 = malloc(sizeof(line_t));

			//numéro de la ligne
			ligne_1->num_line = buf[1];

			// Longueur des lignes du support original							
			k = buf[3];
			
			// on met la longueur de cette ligne
			ligne_1->size=k;
			
			// on met les valeurs de la ligne reçue dans notre ligne			
			for (j = 0; j < k; j ++){
				ligne_1->val[j]=buf[j + 4];
			}
			
			// On met la ligne reçue dans notre tableau de pointeurs
			lignes_rec[ligne_1->num_line]= ligne_1;
			
			// Impression du datagramme reçu sur le terminal
			fprintf(stdout, "---------------------------------------\n");
			fprintf(stdout, "Le datagramme reçu est une ligne\n");
			fprintf(stdout, "l : %d \n", l);				
			fprintf(stdout, "k : %d \n", k);
			fprintf(stdout, "numéro de la ligne : %d \n", ligne_1->num_line);
			fprintf(stdout, "longueur de la ligne : %d \n", ligne_1->size);
			fprintf(stdout, "ligne reçue : \n");
			for (j = 0; j < ligne_1->size; j ++)
			{
				fprintf(stdout, "%d ", ligne_1->val[j]);
			}
			fprintf(stdout, "\n");
			fprintf(stdout, "nb d'octets du datagramme reçu : %d \n", n);
			fprintf(stdout, "---------------------------------------\n");
			fprintf(stdout, "Nombre de lignes recues : %d \n", nb_ligne_rec);
		}

		//si le datagramme est une projection
		if(type_data == 'p'){

			// Le nombre de projections envoyées par l'émetteur
			nb_projections = buf[2];			
			
			// Si c'est la première projection reçue alors on alloue la mémoire nécessaire
			// pour stocker toutes les projections
			if ( nb_projec_rec == 0){
				projections_rec = malloc(nb_projections * sizeof(projection_t*));
			}

			//on incrémente nb_projec_rec
			nb_projec_rec ++;
		
			//On regarde l'angle de la projection (p et q) ainsi que la taille du support pour ce message (l et k)
			//On peut maintenant calculer l'espace nécessaire pour cette projection

			int p = buf[4]; //angle p
			
			int q = buf[5]; //angle q
			
			l = buf[6]; // l
			
			k = buf[7]; // k			

			//création du support nécessaire pour notre projection
			projections_rec[nb_projec_rec-1] = projection_new(p, q, l, k);

			//Taille de la projection
			int taille_projec = buf[3];			

			//remplissage du support de la projection
			for (j = 0; j < taille_projec; j ++){
				projections_rec[nb_projec_rec-1]->bins[j]=buf[j + 8];
			}

			// Impression du datagramme reçu sur le terminal
			fprintf(stdout, "---------------------------------------\n");
			fprintf(stdout, "Le datagramme reçu est une projection\n");
			fprintf(stdout, "nombre de projections : %d \n", nb_projections);
			fprintf(stdout, "angle p : %d \n", p);
			fprintf(stdout, "angle q : %d \n", q);
			fprintf(stdout, "l : %d \n", l);
			fprintf(stdout, "k : %d \n", k);
			fprintf(stdout, "taille de la projection : %d \n", taille_projec);
			fprintf(stdout, "projetée reçue : \n");
			for (j = 0; j < taille_projec; j ++){
				fprintf(stdout, "%d ", projections_rec[nb_projec_rec-1]->bins[j]);
			}
			fprintf(stdout, "\n");
			fprintf(stdout, "nb d'octets du datagramme recu : %d \n", n);
			fprintf(stdout, "---------------------------------------\n");
			
		}

		//si on a recu toutes les projections on fait la mojette inversé
		//ou même avant si le critère de Katz est vérifié à faire
		if ( l == nb_projec_rec)
		{					
			//création du support
			support_rec = support_new(l, k);
			
			//On appelle la transformation mojette inverse
			inverse_system(support_rec, projections_rec, nb_projec_rec, lignes_rec, nb_ligne_rec);
		
			//Impression
			fprintf(stdout, "Nombre de projections recues : %d \n", nb_projec_rec);
			fprintf(stdout, "Nombre de lignes recues : %d \n", nb_ligne_rec);
			fprintf(stdout, "message inverse mojette (que des projections) : \n");
			fprintf(stdout, "\n");
			for (i = 0; i < (l*k); i ++){
    			fprintf(stdout, "%c", support_rec->pixels[i]);
			}
			fprintf(stdout, "\n");

			//Fin du timer
			time(&time_2);
			total = difftime(time_2,time_1);
			printf("\ntemps : = %lf \n", total);
			
			//on remet à 0
			nb_projec_rec = 0;
			nb_data_rec = 0;
			nb_ligne_rec = 0;
			support_free(support_rec);
		}
		
		//si on a recu toute les lignes
		if ( nb_ligne_rec == l)
		{			
			//création du support
			support_rec = support_new(l, k);

			//On appelle la transformation mojette inverse
			inverse_system(support_rec, projections_rec, nb_projec_rec, lignes_rec, nb_ligne_rec);

			//Impression
			fprintf(stdout, "Nombre de projections reçues : %d \n", nb_projec_rec);
			fprintf(stdout, "Nombre de lignes reçues : %d \n", nb_ligne_rec);
			fprintf(stdout, "message inversé mojette (obtenu avec toutes les lignes) : \n");
			fprintf(stdout, "\n");
			for (i = 0; i < (l*k); i ++){
    			fprintf(stdout, "%c", support_rec->pixels[i]);
			}
			fprintf(stdout, "\n");

			//Fin du timer
			time(&time_2);
			total = difftime(time_2,time_1);
			printf("\ntemps : = %lf \n", total);

			//on remet à 0
			nb_projec_rec = 0;
			nb_data_rec = 0;
			nb_ligne_rec = 0;
			support_free(support_rec);
			
		}

		//si on a recu une ligne et plusieurs projections
		if ( (nb_ligne_rec + nb_projec_rec == l) && nb_ligne_rec != l)
		{				
			//création du support
			support_rec = support_new(l, k);

			//On appelle la transformation mojette inverse
			inverse_system(support_rec, projections_rec, nb_projec_rec, lignes_rec, nb_ligne_rec);
			
			//Impression
			fprintf(stdout, "Nombre de projections reçues : %d \n", nb_projec_rec);
			fprintf(stdout, "Nombre de lignes reçues : %d \n", nb_ligne_rec);
			fprintf(stdout, "\n");
			fprintf(stdout, "message inversé mojette (systematique) : \n \n");
			for (i = 0; i < (l*k); i ++){
    		fprintf(stdout, "%c", support_rec->pixels[i]);}
			fprintf(stdout, "\n");

			//Fin du timer
			time(&time_2);
			total = difftime(time_2,time_1);
			printf("\ntemps : = %lf \n", total); 

			//on remet à 0
			nb_projec_rec = 0;
			nb_data_rec = 0;
			nb_ligne_rec = 0;
			support_free(support_rec);
		}
	}
 }
