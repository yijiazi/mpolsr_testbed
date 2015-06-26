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
*******************************************************************/

/*******************************************************************************************
* PROJET : Visio Server
********************************************************************************************
*
* MODULE   : Librairie LibPosp
* AUTEUR   : Author: P.Lesage
* FICHIER  : $Source: //SRVKEONAS/dev/produits/VisioServer/cotu/echanges/srv_echange/libposp/rcs/ConfigFile.cc $
* VERSION  : $Revision: 1.3 $ - $Author: ple $
*
============================================================================================
* informartions :
* $Log: ConfigFile.cc $
* Revision 1.3  2007/08/29 14:18:34Z  ple
* Revision 1.2  2007/08/28 13:15:22Z  ple
* Revision 1.1  2007/08/28 07:48:07Z  ple
* Initial revision
*
*
============================================================================================
*/

#include "configfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include "common.h"
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

map<string, void*>  ConfigFile::m_dicoConfigFile;

ConfigFile::ConfigFile(const char*  szFileName)
{
    m_strFileName = szFileName;
}

ConfigFile::~ConfigFile()
{
    m_dicoValue.clear();
}


void ConfigFile::Initialise()
{

}


void ConfigFile::Uninitialise()
{
    ConfigFile* pTmp;

    for(map<string, void*>::iterator iter= m_dicoConfigFile.begin(); iter != m_dicoConfigFile.end();++iter)
    {
        pTmp = (ConfigFile*)(iter->second);
        delete pTmp;
    }

    m_dicoConfigFile.clear();
}

void ConfigFile::DumpConfig()
{
    for(map<string, string>::iterator iter= m_dicoValue.begin(); iter != m_dicoValue.end();++iter)
    {
        kprintf("param : %s, valeur : %s\n", iter->first.c_str(), iter->second.c_str());
    }

}



bool ConfigFile::GetIntValue(const char* szKey, int& nValue)
{
    string strKey(szKey);

    map<string, string>::iterator iter = m_dicoValue.find(strKey);

    if(m_dicoValue.count(strKey) == 0)
        return false;

    nValue = atoi(iter->second.c_str());
    return true;
}


bool ConfigFile::GetStringValue(const char* szKey, string& strValue)
{
    string strKey(szKey);

    map<string, string>::iterator iter = m_dicoValue.find(strKey);

    if(m_dicoValue.count(strKey) == 0)
        return false;

    strValue = iter->second;
    return true;
}

void* ConfigFile::GetConfigFile(const char* szFileName, bool bEnCache /*= true*/)
{
    FILE*       configFile;
    int         lineno = 0;       /* line counter */

    bool        bConfigOK = true;
    char        rcline[512],      /* line in configuration file */
                mnemonic[512],     /* mnemonic in line */
                *valueptr,
                *prcline;        /* pointer to value list */

    ConfigFile* pNewConfig;
    string strFileName(szFileName);

    if(bEnCache)
    {
        // on recherche si la config est deja en memoire
        map<string, void*>::iterator iter = m_dicoConfigFile.find(strFileName);

        if(m_dicoConfigFile.count(strFileName) == 1)
        {
            // config deja chargee
            return iter->second;
        }
    }

    pNewConfig = new ConfigFile(szFileName);

    // la config n'est pas encore chargee
    if ((configFile = fopen(szFileName, "r")) == NULL) 
    {
        MSG_FAILED("cannot open configuration file : %s",szFileName);
        delete pNewConfig;
        return NULL;
    }


    while (!feof(configFile))
    {
        prcline = fgets(rcline, sizeof(rcline), configFile); /* read line in configuration file */

        if(prcline == NULL)
        {
            // end of file or error
            if (feof(configFile)) 
                break; // fin du fichier de config

            // pb de lecture
            MSG_FAILED("cannot read data configuration file : %s",szFileName);
            bConfigOK = false;
            break;
        }

        lineno++;

        prcline = pNewConfig->RemoveBlank(rcline);

        if (prcline[0] == 0 || prcline[0] == '#' || prcline[0] == 10 || prcline[0] == 13)
            continue;        // ligne de commentaire ou ligne vide

        // recuperation du debut de la ligne (ie la cle)
        char* pos = strstr(prcline,"=");
        if(pos == NULL)
        {
            // pas de signe '='  => chaine non valide
            MSG_FAILED("invalid data : '%s' in configuration file : %s",prcline, szFileName);
            continue;
        }

        memset(mnemonic,0,512);
        memcpy(mnemonic,prcline,(pos-prcline));
        string strKey = pNewConfig->RemoveBlank(mnemonic);

        // recuperation de la donnee
        valueptr = pNewConfig->SkipEqual(pos+1);  // passage de la cle pour retouver les donnees

        string strValue = pNewConfig->RemoveQuote(valueptr);

        pNewConfig->m_dicoValue.insert(pair<string,string>(strKey,strValue));

    }

    fclose(configFile);

    if(!bConfigOK)
    {
        delete pNewConfig;
        pNewConfig = NULL;
    }
    else
    {
        if(bEnCache)
            m_dicoConfigFile.insert(pair<string,void*>(strFileName,pNewConfig));
    }

    return pNewConfig;
}

char* ConfigFile::RemoveBlank(char* szBuffer)
{
    char* szTmp = szBuffer;
    char* szEnd;

    // suppression des espaces en debut de ligne
    while(szTmp[0]==' ')
        szTmp++;

    // supprreessiioonn des espaces en fin de ligne
    szEnd = szTmp;
    szEnd += strlen(szTmp);
    if(szEnd > szTmp)
    {
        szEnd--; // pour le 0 de fin de chaine

        while((szEnd[0] == 13) || (szEnd[0] == 10) || (szEnd[0] == ' '))
        {
            szEnd[0]=0;
            szEnd--;

            if(szEnd <= szTmp)
                break;
        }

    }

    return szTmp;

}

char* ConfigFile::RemoveQuote(char* szBuffer)
{
    char* szTmp = szBuffer;
    char* szEnd;

    // suppression des espaces en debut de ligne
    while((szTmp[0]==' ') ||(szTmp[0]=='"'))
        szTmp++;

    // supprreessiioonn des espaces en fin de ligne
    szEnd = szTmp;
    szEnd += strlen(szTmp);
    if(szEnd > szTmp)
    {
        szEnd--; // pour le 0 de fin de chaine

        while((szEnd[0]==' ') ||(szEnd[0]=='"'))
        {
            szEnd[0]=0;
            szEnd--;

            if(szEnd <= szTmp)
                break;
        }

    }

    return szTmp;

}

char* ConfigFile::SkipEqual(char* szBuffer)
{
    char* szTmp = szBuffer;

    // suppression des espaces en debut de ligne
    while((szTmp[0]==' ') || (szTmp[0]=='='))
        szTmp++;

    return szTmp;
}

