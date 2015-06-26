#ifndef OLSR_EVENT_CLIENT_H
#define OLSR_EVENT_CLIENT_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "olsreventmessage.h"
#include "tcpconnection.h"

using boost::asio::ip::tcp;
using boost::asio::io_service;

namespace sereadmo {
namespace olsrclient {

class OlsrEventClient
{
public:
  OlsrEventClient(boost::asio::io_service& io_service);
  OlsrEventClient(boost::asio::io_service& io_service, tcp::endpoint& endpoint);

  // Connects to OLSR event server
  bool connect(tcp::endpoint& endpoint);
  // Close the connection
  void  close();

private:

  void handleConnect(const boost::system::error_code& error);
  void handleRead(const boost::system::error_code& error);
  void handleWrite(const boost::system::error_code& error);

  void doClose();

private:
  boost::asio::io_service&  m_io_service;
  TcpConnectionPtr    m_tcpConnection;
  OlsrEventMessage        m_messageRead;
};

}
}

#endif
