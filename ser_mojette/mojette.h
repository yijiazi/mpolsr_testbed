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


#ifndef MOJETTE_H_
#define MOJETTE_H_

/**
 * mojette support
 */
typedef struct support {
    int l;
    int k;
    char* pixels;
} support_t;

/**
 * mojette projection
 */
typedef struct projection {
    int p;
    int q;
    char* bins;
    int size;
} projection_t;


/**
 * line
 */
typedef struct line {
    int num_line;
    char val[1024];
    int size;
} line_t;

/**
 * alloc a new support k, l
 */
support_t* support_new(int l, int k);

/**
 * free the support
 */
void support_free(support_t* support);

void line_free(line_t* line);

/**
 * alloc a new projection of angle p, q for support k, l
 */
projection_t* projection_new(int p, int q, int l, int k);

/**
 * free the projection
 */
void projection_free(projection_t* projection);

/**
 * perform a forward mojette transform (xor)
 */
void forward(support_t* support, projection_t** projections, int nr_prj);

/**
 * perform an inverse mojette transform (xor)
 */
void inverse_system(support_t* support, projection_t** projections, int nr_prj, line_t** line, int nb_ligne);

/**
 * perform an inverse mojette transform (xor)
 */
void inverse(support_t* support, projection_t** projections, int nr_prj);

#endif /*MOJETTE_H_*/
