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

#ifndef OLSR_EVENT_MSG_H
#define OLSR_EVENT_MSG_H

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <deque>
#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>

// token sent from OLSR plugin to sereadmo-netfilter module
class OlsrEventMessage
{
  public:
    long    type;
    unsigned long   addr_1;
    unsigned long   addr_2;
    int     value;


  template<class Archive>
  void serialize ( Archive& ar, const unsigned int version )
  {
    ar & type & addr_1 & addr_2 & value;
  }

  enum {
    DEV_CMD_RESET_TC       = 1,
    DEV_CMD_ADD_NODE       = 2,
    DEV_CMD_REMOVE_NODE    = 3,
    DEV_CMD_ADD_LINK       = 4,
    DEV_CMD_REMOVE_LINK    = 5,
    DEV_CMD_ADD_ALIAS      = 6,
    DEV_CMD_REMOVE_ALIAS   = 7,
    DEV_CMD_SET_LOCAL_ADDR = 8,
    DEV_CMD_SET_ALIAS_ADDR = 9
  };
};

typedef std::deque<OlsrEventMessage> OlsrEventMessageQueue;

#endif
