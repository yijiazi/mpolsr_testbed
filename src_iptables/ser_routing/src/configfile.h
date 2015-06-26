/*******************************************************************************************
* PROJET : Visio Server
********************************************************************************************
*
* MODULE   : Librairie LibPosp
* AUTEUR   : Author: P.Lesage
* FICHIER  : $Source: //SRVKEONAS/dev/produits/VisioServer/cotu/echanges/srv_echange/libposp/rcs/ConfigFile.h $
* VERSION  : $Revision: 1.3 $ - $Author: ple $
*
============================================================================================
* informartions :
* $Log: ConfigFile.h $
* Revision 1.3  2008/03/26 18:12:45Z  ple
* Modifications pour compilation sous Windows
* Revision 1.2  2007/08/28 13:15:23Z  ple
* Revision 1.1  2007/08/28 07:48:09Z  ple
* Initial revision
*
*
============================================================================================
* Used and modified 2015
*   by      Benjamin Mollé (engineering student, Polytech Nantes, University of Nantes, France) benjamin.molle@gmail.com
*           Denis Souron (engineering student, Polytech Nantes, Université of Nantes, France) denis.souron@laposte.net
*/
#if !defined(_CONFIGFILE_H_)
#define _CONFIGFILE_H_

#ifdef WIN32
    #pragma warning( disable : 4786)
#endif


#include <string>
#include <map>

using namespace std;

#define CONFIG_DBHOST           "DBHost"
#define CONFIG_DBNAME           "DBName"
#define CONFIG_DBUSER           "DBUser"
#define CONFIG_DBPASSWORD       "DBPassword"

class ConfigFile
{

protected:
    static map<string, void*>   m_dicoConfigFile;
    map<string, string>         m_dicoValue;
    string                      m_strFileName;

protected:

    char* RemoveBlank(char* szBuffer);
    char* SkipEqual(char* szBuffer);
    char* RemoveQuote(char* szBuffer);

    ConfigFile(const char*  szFileName);

public:

    virtual ~ConfigFile();

    static void Initialise();
    static void Uninitialise();

    static void* GetConfigFile(const char* szFileName, bool bEnCache = true);

    bool GetIntValue(const char* szKey, int& nValue);
    bool GetStringValue(const char* szKey, string& strValue);

    void DumpConfig();
};

#endif // !defined(_CONFIGFILE_H_)
