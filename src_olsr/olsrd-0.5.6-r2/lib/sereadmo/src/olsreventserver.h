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

//
// C++ Interface: olsreventserver
//
// Description: 
//
//
// Author:  <root@bt>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef OLSREVENTSERVER_H
#define OLSREVENTSERVER_H


#include <boost/asio.hpp>
#include "eventlistener.h"

using boost::asio::ip::tcp;

/**
	@author  <root@bt>
*/
class OlsrEventServer{
public:
    OlsrEventServer(boost::asio::io_service& io_service, short port);
    
    void addMessage(OlsrEventMessage msg);
    void notify();
    
private:
  void waitForConnection();
  void handleAccept(TcpConnectionPtr eventListener, const boost::system::error_code& error);
    
  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
  EventListenerManager eventListenerManager_;
  
  OlsrEventMessageQueue msgQueue_;
};

typedef boost::shared_ptr<OlsrEventServer> OlsrEventServerPtr;

#endif
