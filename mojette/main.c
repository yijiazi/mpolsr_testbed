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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mojette.h"

//XXX : errors.
int main(int argc, char **argv) {
/*
    FILE* in;
    FILE* out;
    support_t* support;
    support_t* support_rec;
    projection_t** projections;
    projection_t** projections_rec;
    
    // Forward.
    support = support_new(512, 512);
    
    projections = malloc(3 * sizeof(projection_t*));
    projections[0] = projection_new(1, 256, 512, 512);
    projections[1] = projection_new(1, 257, 512, 512);
    projections[2] = projection_new(1, 258, 512, 512);
    
    in = fopen("./lena.raw", "r");
    fread((unsigned char*)support->pixels, sizeof(unsigned char), support->l * support->k, in);
    fclose(in);
    
    forward(support, projections, 3);
    
    fprintf(stdout, "OK 2\n");
    // Inverse.
    support_rec = support_new(512, 512);
    fprintf(stdout, "OK 2.2\n");
    
    projections_rec = malloc(2 * sizeof(projection_t*));
    projections_rec[0] = projections[0];
    projections_rec[1] = projections[2];
    fprintf(stdout, "OK 2.3\n");
    
    inverse(support_rec, projections_rec, 2);
    fprintf(stdout, "OK 2.4\n");
    
    out = fopen("lena.rec", "w");
    fwrite((unsigned char*)support_rec->pixels, sizeof(unsigned char), support_rec->l * support_rec->k, 
        out);
    fclose(out);
	
*/


    char tmp[9] = {4, 2, 1, 8, 2, 3, 5, 4, 6}; // Matrice exemple des spécification de SERADMO 
	
    support_t* support;
    support_t* support_rec;
    projection_t** projections;
    projection_t** projections_rec;
    int i, j;
    
    // Forward.
    support = support_new(3, 3); // Initialisation du support de projection (ici matrice 3x3)
    memcpy(support->pixels, tmp, 9);
	printf("Initial : ");
    for (i = 0; i < 9; i ++)
            fprintf(stdout, "%d ", support->pixels[i]);
    fprintf(stdout, "\n");
	printf("Matrice : \n");

    for (i = 0; i < 3; i ++)
    {
        for (j = 0; j < 3; j ++)
            fprintf(stdout, "%d ", support->pixels[i * support->k + j]);
        fprintf(stdout, "\n");
    }
    
    projections = malloc(3 * sizeof(projection_t*));
    projections[0] = projection_new(-1, 1, 3, 3);
    projections[1] = projection_new( 1, 0, 3, 3);
    projections[2] = projection_new( 1, 1, 3, 3);
    
    
    forward(support, projections, 3);
    
    for (i = 0; i < 3; i ++)
    {
		fprintf(stdout, "Projection %d,%d:  taille : %d | ", projections[i]->p, projections[i]->q, projections[i]->size);

        for (j = 0; j < projections[i]->size; j ++){
			fprintf(stdout, "%d ", projections[i]->bins[j]);
		}
	
			fprintf(stdout, "\n");

    }
    
    // Inverse.
	printf("Nettoyage du support : \n");

    support_rec = support_new(3, 3);
    for (i = 0; i < 3; i ++)
    {
	for (j = 0; j < 3 ;j ++)
            fprintf(stdout, "%d ", support_rec->pixels[i * support->k + j]);
        fprintf(stdout, "\n");
    }
    
    projections_rec = malloc(4 * sizeof(projection_t*));
    projections_rec[0] = projections[0];
    projections_rec[1] = projections[1];
	projections_rec[2] = projections[2];

    
    inverse(support_rec, projections_rec, 3);
	printf("Décodé : \n");

    for (i = 0; i < 9; i ++)
            fprintf(stdout, "%d ", support_rec->pixels[i]);
    fprintf(stdout, "\n");
    
	printf("Matrice reconstruite : \n");

    for (i = 0; i < 3; i ++)
    {
        for (j = 0; j < 3; j ++)
            fprintf(stdout, "%d ", support_rec->pixels[i * support->k + j]);
        fprintf(stdout, "\n");
    }

    return 0;
}