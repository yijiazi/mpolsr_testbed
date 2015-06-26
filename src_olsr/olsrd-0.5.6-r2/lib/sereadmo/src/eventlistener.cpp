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
// C++ Implementation: eventlistener
//
// Description: 
//
//
// Author:  <root@bt>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "eventlistener.h"
extern "C" {
#include "ser_global.h"
}
static char readChar;

EventListener::EventListener(TcpConnectionPtr tcpConnection, EventListenerManager& eventListenerManager)
    :  tcpConnection_(tcpConnection), eventListenerManager_(eventListenerManager)
{
}


EventListener::~EventListener()
{
}

void EventListener::start()
{
  eventListenerManager_.subscribe(shared_from_this());

  // Only to detect connection loss
  waitForData();
}

void EventListener::waitForData()
{
  boost::asio::async_read(tcpConnection_->socket(),
      boost::asio::buffer(&readChar, 1),
      boost::bind(
        &EventListener::handleRead, shared_from_this(),
        boost::asio::placeholders::error));
}

void EventListener::notifyEvent(const OlsrEventMessage& eventMessage)
{
    //SER_PRINTF(">>> EventListener::notifyEvent\n");
    bool write_in_progress = !writeMsgs_.empty();
    writeMsgs_.push_back(eventMessage);
    if (!write_in_progress)
    {
      SER_PRINTF("Dispatching message type: %d", eventMessage.type);
      tcpConnection_->async_write(writeMsgs_.front(),
          boost::bind(&EventListener::handleWrite, shared_from_this(),
            boost::asio::placeholders::error));
    }
    //SER_PRINTF("<<< EventListener::notifyEvent\n");
}

void EventListener::handleRead(const boost::system::error_code& error)
{
  if (!error)
  {
    waitForData();
  }
  else
  {
   SER_PRINTF("Lost connection");

    eventListenerManager_.unsubscribe(shared_from_this());
  }
}

void EventListener::handleWrite(const boost::system::error_code& error)
{
  //SER_PRINTF(">>> EventListener::handleWrite\n");
  if (!error)
  {
    writeMsgs_.pop_front();
    if (!writeMsgs_.empty())
    {
      SER_PRINTF("Dispatching message type: %d", writeMsgs_.front().type);
      tcpConnection_->async_write(writeMsgs_.front(),
          boost::bind(&EventListener::handleWrite, shared_from_this(),
          boost::asio::placeholders::error));
    }
  }
  else
  {
    eventListenerManager_.unsubscribe(shared_from_this());
  }
  //SER_PRINTF("<<< EventListener::handleWrite\n");
}
