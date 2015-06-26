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
// C++ Implementation: olsreventserver
//
// Description: 
//
//
// Author:  <root@bt>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "olsreventserver.h"
extern "C" {
#include "ser_global.h"
#include "ser_device.h"
#include "ser_tc.h"
}

OlsrEventServer::OlsrEventServer(boost::asio::io_service& io_service, short port)
    : io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
  {
  
    //std::cout << "OLSR event server listening on port " << port << std::endl;
    SER_PRINTF("OLSR event server listening on port  %d",port);
    waitForConnection();
  }

// Attente d'un nouveau client
void OlsrEventServer::waitForConnection()
{
    TcpConnectionPtr newConnection(new TcpConnection(io_service_));

    // Attente d'une nouvelle connection
    acceptor_.async_accept(newConnection->socket(),
        boost::bind(&OlsrEventServer::handleAccept, this,
        newConnection, boost::asio::placeholders::error)
        );
}

void OlsrEventServer::handleAccept(TcpConnectionPtr connectionPtr,
      const boost::system::error_code& error)
{
  if (!error)
  {
    SER_PRINTF("Got new connection");
    EventListenerPtr newListener(new EventListener(connectionPtr, eventListenerManager_));
    newListener->start();
    dev_send_init();
    tc_send_all_data();
    dev_send();
    // Waiting for new connection again
    waitForConnection();
  }
}

void OlsrEventServer::addMessage(OlsrEventMessage msg)
{
  msgQueue_.push_back(msg);
}

void OlsrEventServer::notify()
{
    //SER_PRINTF(">>> OlsrEventServer::notify");

    while(!msgQueue_.empty())
    {
      eventListenerManager_.notifyEvent(msgQueue_.front());
      msgQueue_.pop_front();
    }
    //SER_PRINTF("<<< OlsrEventServer::notify");
}



