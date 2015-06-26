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
* Used and modified 2015
*   by      Benjamin Mollé (engineering student, Polytech Nantes, University of Nantes, France) benjamin.molle@gmail.com
*           Denis Souron (engineering student, Polytech Nantes, Université of Nantes, France) denis.souron@laposte.net
******************************************************************

*
=========================================================================
*
* Test application to validate char device communication from user app
* to sereadmo netfilter module
*
*************************************************************************
* Changes :
* --------
*
*************************************************************************/
#include <stdio.h>
#include <string.h>
#include <errno.h>


//***********************************************************************
//***********************************************************************



#define DEVICE_NAME  "/dev/sereadmo"


#define DEV_CMD_RESET_TC        1
#define DEV_CMD_ADD_NODE        2
#define DEV_CMD_REMOVE_NODE     3
#define DEV_CMD_ADD_LINK        4
#define DEV_CMD_REMOVE_LINK     5
#define DEV_CMD_ADD_ALIAS       6
#define DEV_CMD_REMOVE_ALIAS    7
#define DEV_CMD_SET_LOCAL_ADDR  8
#define DEV_CMD_SET_ALIAS_ADDR  9

struct dev_tc_token
{
    long            type;
    unsigned long   addr_1;
    unsigned long   addr_2;
    int             value;
};

FILE*                   g_device = NULL;


//***********************************************************************
//***********************************************************************



static int send_cmd(int type, unsigned long   addr_1,unsigned long   addr_2, int value)
{
    struct dev_tc_token     token;

    memset(&token,0,sizeof(struct dev_tc_token));
    token.type = type;
    token.addr_1 = addr_1;
    token.addr_2 = addr_2;
    token.value = value;

    if(fwrite(&token,sizeof(struct dev_tc_token),1,g_device) != 1)
    {
        printf("Failed to send cmd : %d\n",type);
        return -1;
    }

    return 0;
}


//***********************************************************************
//***********************************************************************



int main() {

    char    buffer[200];
    int     index;

    if ((g_device = fopen(DEVICE_NAME,"r+b")) == 0) 
    {
        printf("Can't open : %s -> %s\n", DEVICE_NAME, strerror(errno)); 
        return 1;
    }

    memset(buffer,0,200);
    fread(buffer,199,1,g_device);
    printf("Read from device : %s\n",buffer);


    // send init TC
    //-------------
    send_cmd(DEV_CMD_RESET_TC,0,0,0);


    // add nodes
    //----------
    for(index = 1; index < 6; index++)
        send_cmd(DEV_CMD_ADD_NODE,index,0,0);


    // add links
    //----------
    for(index = 2; index < 6; index++)
        send_cmd(DEV_CMD_ADD_LINK,index-1,index,0);

    for(index = 3; index < 6; index+=2)
        send_cmd(DEV_CMD_ADD_LINK,index-2,index,0);


    send_cmd(DEV_CMD_REMOVE_NODE,3,0,0);

    fclose(g_device);

    return 0;
}

