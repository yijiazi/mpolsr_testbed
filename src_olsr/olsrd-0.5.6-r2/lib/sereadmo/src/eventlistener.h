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
// C++ Interface: eventlistener
//
// Description: 
//
//
// Author:  <root@bt>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef EVENTLISTENER_H
#define EVENTLISTENER_H

#include <list>
#include <set>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

#include "olsreventmessage.h"
#include "connection.h"

using boost::asio::ip::tcp;

class EventListenerManager;

//----------------------------------------------------------------------
class EventListener 
  : public boost::enable_shared_from_this<EventListener>
{
public:
    EventListener(TcpConnectionPtr connection, EventListenerManager& eventListenerManager);

    ~EventListener();

    void start();
    void waitForData();
    virtual void notifyEvent(const OlsrEventMessage& eventMessage);

    void handleRead(const boost::system::error_code& error);
    void handleWrite(const boost::system::error_code& error);
private:
  TcpConnectionPtr tcpConnection_;
  EventListenerManager& eventListenerManager_;
  OlsrEventMessageQueue writeMsgs_;    
};

typedef boost::shared_ptr<EventListener> EventListenerPtr;

//----------------------------------------------------------------------
class EventListenerManager
{
public:
  EventListenerManager(){};
  
  void subscribe(const EventListenerPtr& listener)
  {
    //std::cout << ">>> EventListenerManager::subscribe" << std::endl;
    eventListeners_.insert(listener);
    //std::cout << "<<< EventListenerManager::subscribe" << std::endl;
  }
  
  void unsubscribe(const EventListenerPtr& listener)
  {
    eventListeners_.erase(listener);
  }
  
  virtual void notifyEvent(const OlsrEventMessage& eventMessage)
  {
    //std::cout << ">>> EventListenerManager::notifyEvent" << std::endl;

    std::for_each(eventListeners_.begin(), eventListeners_.end(),
        boost::bind(&EventListener::notifyEvent, _1, boost::ref(eventMessage)));  
    
    //std::cout << "<<< EventListenerManager::notifyEvent" << std::endl;
  }
  
protected:
  std::set<EventListenerPtr> eventListeners_;
};


#endif
