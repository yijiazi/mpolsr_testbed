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
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mojette.h"

projection_t* projection_new(int p, int q, int l, int k)
{
    projection_t* new;
    
    if ((new = malloc(sizeof(projection_t))) == NULL)		    
        return NULL;
        
    new->p = p;
    new->q = q;
    new->size = abs(p) * (l - 1) + abs(q) * (k - 1) + 1;
    if ((new->bins = calloc(new->size, sizeof(char))) == NULL)
    {
        free(new);
        return NULL;
    }
    
    return new;
}



void projection_free(projection_t* projection)
{
    if (projection != NULL)
    {
        if (projection->bins != NULL)
            free(projection->bins);
        
        free(projection);
    }
}

void line_free(line_t* line)
{
    if (line != NULL)
    {
        if (line->val != NULL)
            free(line->val);
        
        free(line);
    }
}

support_t* support_new(int l, int k)
{
    support_t* new;
    
    if ((new = malloc(sizeof(support_t))) == NULL)
        return NULL;
    
    new->l = l;
    new->k = k;
    if ((new->pixels = calloc(l * k, sizeof(char))) == NULL)
    {
        free(new);
        return NULL;
    }
    
    return new;
}

void support_free(support_t* support)
{
    if (support != NULL)
    {
        if (support->pixels != NULL)
            free(support->pixels);
        
        free(support);
    }
}

void forward(support_t* support, projection_t** projections, int nr_prj)
{
    int l;
    int k;
    projection_t** pp;
    
    for (l = 0; l < support->l; l ++)
    {
        for (k = 0; k < support->k; k ++)
        {
            for (pp = projections; pp < projections + nr_prj; pp ++)
            {
                int offset;
                projection_t* p;
                
                p = *pp;
                offset = p->p < 0 ? (support->l - 1) * p->p : 0;
                p->bins[k * p->q + l * p->p - offset] ^= 
                    support->pixels[l * support->k + k];
            }
        }
    }
}

typedef struct univoc {
    struct univoc* next;
    unsigned long* u;
    unsigned long* d;
    char* p;
} univoc_t;

void inverse_system(support_t* support, projection_t** projections, int nr_prj, line_t** ligne, int nb_ligne)
{
    int i;
    int k;
    int l;
    long d;
    unsigned long** pu;
    unsigned long** pd;
    univoc_t* curr;
    univoc_t* head;
    univoc_t* tail;
    int nb_pixel;

	//si on a pas de projection alors on remplit directement le support
	if(nr_prj==0 && support->l==nb_ligne){
		for(i = 0; i < nb_ligne; i ++)
		{
			for (k = 0; k < (support->k); k ++)
			{
				int j;
				support->pixels[(ligne[i]->num_line) * support->k + k] = ligne[i]->val[k];
			}
		}
	
	}else{
    
    	pu = malloc(nr_prj * sizeof(unsigned long*));
   		pd = malloc(nr_prj * sizeof(unsigned long*));

   		for (i = 0; i < nr_prj; i ++)
    	{
        	projection_t* p;
       		int size;
        
        	p = projections[i];
        	size = abs(p->p) * (support->l - 1) + abs(p->q) * (support->k - 1) + 1;
        	pu[i] = calloc(size, sizeof(unsigned long));
        	pd[i] = calloc(size, sizeof(unsigned long));
    	}
    
    	d = 0;
    	for (l = 0; l < support->l; l++)
    	{
        	for (k = 0; k < support->k; k++)
        	{
            	for (i = 0; i < nr_prj; i ++)
            	{
                	int offset = projections[i]->p < 0 ? 
                    	(support->l - 1) * projections[i]->p : 0;
                	int b = k * projections[i]->q + l * projections[i]->p - offset;
                	pu[i][b] += 1;
                	pd[i][b] += d;
            	}
            	d ++;
        	}
    	}
	
		unsigned long diet;
		for(i = 0; i < nb_ligne; i ++){
			for (k = 0; k < (support->k); k ++)
			{
				int j;
				support->pixels[(ligne[i]->num_line) * support->k + k] = ligne[i]->val[k];
				diet = k + (ligne[i]->num_line) * support->k;
				for (j = 0; j < nr_prj; j ++)
				{
					//on calcule le offset
				     int offset = projections[j]->p < 0 ? 
				         (support->l - 1) * projections[j]->p : 0;

					//on calcule l'indice du bin de la projection impacté
				     int b = k * projections[j]->q + (ligne[i]->num_line) * projections[j]->p - offset;

				     // on met à jour le bin de la projection
				     projections[j]->bins[b] ^= ligne[i]->val[k];

					// on met à jour le diet
				     pd[j][b] -= diet;

					// on met à jour
				     pu[j][b] -= 1; 
				}
			}
		}


    	head = NULL;
    	tail = NULL;
    	for (i = 0; i < nr_prj; i ++)
    	{
        	int j;
        	for (j = 0; j < projections[i]->size; j ++)
        	{
            	if (pu[i][j] == 1)
            	{
                	univoc_t* new = malloc(sizeof(univoc_t));
                	new->next = NULL;
                	new->u = (pu[i]) + j;
                	new->d = (pd[i]) + j;
                	new->p = (projections[i]->bins) + j;

					diet = *(new->d);
					l = (int)(diet / support->k);

        			k = (int)(diet - l * support->k);
                
                	if (! head)
                    	head = new;
                	else
                    	tail->next = new;
                    
                	tail = new;
            	}
        	}
    	}
    
    	nb_pixel = 0;
    	curr = head;
  
    	while (nb_pixel ++ < ((support->l * support->k)-(support->k * nb_ligne)))
    	{
        	int j;
        	char bin;

        	while (*(curr->u) != 1)
            	curr = curr->next;
        
        	bin = *(curr->p);
	
        	diet = *(curr->d);
        
        	l = (int)(diet / support->k);
        	k = (int)(diet - l * support->k);

        	support->pixels[l * support->k + k] = bin;

        	for (j = 0; j < nr_prj; j ++)
        	{
            	int offset = projections[j]->p < 0 ? 
                	(support->l - 1) * projections[j]->p : 0;
            	int b = k * projections[j]->q + l * projections[j]->p - offset;
            
            	projections[j]->bins[b] ^= bin;
            	pd[j][b] -= diet;
            	pu[j][b] -= 1;
            
            	if (pu[j][b] == 1)
            	{
                	univoc_t* new = malloc(sizeof(univoc_t));
                	new->next = NULL;
                	new->u = (pu[j]) + b;
                	new->d = (pd[j]) + b;
                	new->p = (projections[j]->bins) + b;
                
                	tail->next = new;
                	tail = new;
            	}
        	}
    	}
	}
}

void inverse(support_t* support, projection_t** projections, int nr_prj)
{
    int i;
    int k;
    int l;
    long d;
    unsigned long** pu;
    unsigned long** pd;
    univoc_t* curr;
    univoc_t* head;
    univoc_t* tail;
    int nb_pixel;
    
    pu = malloc(nr_prj * sizeof(unsigned long*));
    pd = malloc(nr_prj * sizeof(unsigned long*));
    for (i = 0; i < nr_prj; i ++)
    {
        projection_t* p;
        int size;
        
        p = projections[i];
        size = abs(p->p) * (support->l - 1) + abs(p->q) * (support->k - 1) + 1;
        pu[i] = calloc(size, sizeof(unsigned long));
        pd[i] = calloc(size, sizeof(unsigned long));
    }
    
    d = 0;
    for (l = 0; l < support->l; l++)
    {
        for (k = 0; k < support->k; k++)
        {
            for (i = 0; i < nr_prj; i ++)
            {
                int offset = projections[i]->p < 0 ? 
                    (support->l - 1) * projections[i]->p : 0;
                int b = k * projections[i]->q + l * projections[i]->p - offset;
                pu[i][b] += 1;
                pd[i][b] += d;
            }
            d ++;
        }
    }
    
    head = NULL;
    tail = NULL;
    for (i = 0; i < nr_prj; i ++)
    {
        int j;
        for (j = 0; j < projections[i]->size; j ++)
        {
            if (pu[i][j] == 1)
            {
                univoc_t* new = malloc(sizeof(univoc_t));
                new->next = NULL;
                new->u = (pu[i]) + j;
                new->d = (pd[i]) + j;
                new->p = (projections[i]->bins) + j;
                
                if (! head)
                    head = new;
                else
                    tail->next = new;
                    
                tail = new;
            }
        }
    }
    
    nb_pixel = 0;
    curr = head;
  
    while (nb_pixel ++ < support->l * support->k)
    {
        int j;
        char bin;
        unsigned long diet;
        
        while (*(curr->u) != 1)
            curr = curr->next;
        
        bin = *(curr->p);
        diet = *(curr->d);
        
        l = (int)(diet / support->k);
        k = (int)(diet - l * support->k);

        support->pixels[l * support->k + k] = bin;

        for (j = 0; j < nr_prj; j ++)
        {
            int offset = projections[j]->p < 0 ? 
                (support->l - 1) * projections[j]->p : 0;
            int b = k * projections[j]->q + l * projections[j]->p - offset;
            
            projections[j]->bins[b] ^= bin;
            pd[j][b] -= diet;
            pu[j][b] -= 1;
            
            if (pu[j][b] == 1)
            {
                univoc_t* new = malloc(sizeof(univoc_t));
                new->next = NULL;
                new->u = (pu[j]) + b;
                new->d = (pd[j]) + b;
                new->p = (projections[j]->bins) + b;
                
                tail->next = new;
                tail = new;
            }
        }
    }
	exit(0);
}
