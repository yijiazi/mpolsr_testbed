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

#include <string>
#include "olsreventclient.h"

extern "C" {
#include "common.h"
#include "ser_path.h"
}

using std::string;

namespace sereadmo {
namespace olsrclient {

char* g_tabCmdName[] = {
    "RESET_TC", //       = 1,
    "ADD_NODE", //       = 2,
    "REMOVE_NODE", //    = 3,
    "ADD_LINK", //       = 4,
    "REMOVE_LINK", //    = 5,
    "ADD_ALIAS", //      = 6,
    "REMOVE_ALIAS", //   = 7,
    "SET_LOCAL_ADDR", // = 8,
    "SET_ALIAS_ADDR" // = 9
};


OlsrEventClient::OlsrEventClient (boost::asio::io_service& io_service)
        : m_io_service ( io_service ), m_tcpConnection ( TcpConnectionPtr ( new TcpConnection ( io_service ) ) )
{
}

OlsrEventClient::OlsrEventClient ( boost::asio::io_service& io_service, tcp::endpoint& endpoint )
        : m_io_service ( io_service ), m_tcpConnection ( TcpConnectionPtr ( new TcpConnection ( io_service ) ) )
{
    // Asynchronous connection to server
    tcp::socket& sock = m_tcpConnection->socket();
    sock.async_connect ( endpoint,
                         boost::bind ( &OlsrEventClient::handleConnect, this,
                                       boost::asio::placeholders::error )
                       );
}

bool OlsrEventClient::connect(tcp::endpoint& endpoint )
{
    // Synchronous connection to server
    tcp::socket& sock = m_tcpConnection->socket();
    boost::system::error_code error;
    sock.connect ( endpoint,error );
    if (!error)
    {
        DBG_DEV(1,"Connection with OLSR event server established");
        
        m_tcpConnection->async_read ( m_messageRead,
                                      boost::bind ( &OlsrEventClient::handleRead, this,
                                                    boost::asio::placeholders::error )
                                    );
        return true;
    }
    else
    {
        MSG_FAILED("Could not connect to OLSR event server %s on port %d",
          endpoint.address().to_string().c_str(),
          endpoint.port()
        ); 
    
//       std::cerr << "Could not connect to OLSR event server " << 
//                 << " on port " << endpoint.port() << std::endl;

      if (error==boost::asio::error::connection_refused)
      {
        MSG_FAILED("Please verfiy that OLSR server is running and sereadmo plugin is loaded");
        return false;
      }
      else
        throw boost::system::system_error(error);
    }
}

// Close the connection
void OlsrEventClient::close()
{
    m_io_service.post ( boost::bind ( &OlsrEventClient::doClose, this ) );
}

void OlsrEventClient::handleConnect ( const boost::system::error_code& error )
{
    if ( !error )
    {
        m_tcpConnection->async_read ( m_messageRead,
                                      boost::bind ( &OlsrEventClient::handleRead, this,
                                                    boost::asio::placeholders::error )
                                    );
    }
    else
    {
      MSG_FAILED("Could not connect to OLSR event server");
      throw boost::system::system_error(error);
    }
}


void OlsrEventClient::handleRead ( const boost::system::error_code& error )
{
    string addr1, addr2;
    //std::cout << ">>> OlsrEventClient::handleRead (error " << error << ")" << std::endl;
    if ( !error )
    {
        if (m_messageRead.type>=OlsrEventMessage::DEV_CMD_RESET_TC 
            && m_messageRead.type<=OlsrEventMessage::DEV_CMD_SET_ALIAS_ADDR)
          DBG_DEV(3,"Received event %ld (%s)",m_messageRead.type, g_tabCmdName[m_messageRead.type-1]);
        else
          DBG_DEV(3,"Received unknown event %ld",m_messageRead.type);
        
        switch(m_messageRead.type)
        {
            case OlsrEventMessage::DEV_CMD_RESET_TC :
                tc_free_data(0);
                break;

            case OlsrEventMessage::DEV_CMD_ADD_NODE : 
                DBG_DEV(2,"Add node (%s)", ipToString(m_messageRead.addr_1));
                tc_add_node(m_messageRead.addr_1);
                break;

            case OlsrEventMessage::DEV_CMD_REMOVE_NODE :
                DBG_DEV(2,"Remove node (%s)", ipToString(m_messageRead.addr_1));
                tc_remove_node(m_messageRead.addr_1);
                break;

            case OlsrEventMessage::DEV_CMD_ADD_LINK :
                addr1 = ipToString(m_messageRead.addr_1);
                addr2 = ipToString(m_messageRead.addr_2);
                DBG_DEV(2,"Add link (%s --> %s)", addr1.c_str(), addr2.c_str());
                tc_add_link(m_messageRead.addr_1,m_messageRead.addr_2,m_messageRead.value);
                break;

            case OlsrEventMessage::DEV_CMD_REMOVE_LINK :
                DBG_DEV(2,"Remove link (%s --> %s)", 
                  ipToString(m_messageRead.addr_1),
                  ipToString(m_messageRead.addr_2));
                tc_remove_link(m_messageRead.addr_1,m_messageRead.addr_2);
                break;

            case OlsrEventMessage::DEV_CMD_ADD_ALIAS :
                DBG_DEV(2,"Add alias (%s --> %s)", 
                  ipToString(m_messageRead.addr_1),
                  ipToString(m_messageRead.addr_2));
                tc_add_alias(m_messageRead.addr_1,m_messageRead.addr_2);
                break;

            case OlsrEventMessage::DEV_CMD_REMOVE_ALIAS :
                DBG_DEV(2,"Remove alias (%s --> %s)", 
                  ipToString(m_messageRead.addr_1),
                  ipToString(m_messageRead.addr_2));
                tc_remove_alias(m_messageRead.addr_1,m_messageRead.addr_2);
                break;

            case OlsrEventMessage::DEV_CMD_SET_LOCAL_ADDR :
                DBG_DEV(2,"Set local Ip %s mask %s", 
                  ipToString(m_messageRead.addr_1),
                  ipToString(m_messageRead.addr_2));
                tc_set_local_addr(m_messageRead.addr_1,m_messageRead.addr_2);
                break;

            case OlsrEventMessage::DEV_CMD_SET_ALIAS_ADDR :
                DBG_DEV(2,"Set local alias %s mask %s", 
                  ipToString(m_messageRead.addr_1),
                  ipToString(m_messageRead.addr_2));
                tc_set_local_alias(m_messageRead.addr_1,m_messageRead.addr_2);
                break;
            case OlsrEventMessage::DEV_CMD_SET_DEBUG_LEVEL :
                DBG_DEV(2,"Set debug level to %d for component %d", m_messageRead.addr_2, m_messageRead.addr_1);
                switch(m_messageRead.addr_1)
                {
                case 1:
                  g_dbg_hook = m_messageRead.addr_2; break;
                case 2:
                  g_dbg_device = m_messageRead.addr_2; break;
                case 3:
                  g_dbg_module = m_messageRead.addr_2; break;
                case 4:
                  g_dbg_djk = m_messageRead.addr_2; break;
                default:
                  g_dbg_hook = m_messageRead.addr_2;
                  g_dbg_device = m_messageRead.addr_2;
                  g_dbg_module = m_messageRead.addr_2;
                  g_dbg_djk = m_messageRead.addr_2;
                  break;
                };
                break;
            default:
                MSG_FAILED("SEREADEMO invalid cmd type : %ld",m_messageRead.type);
        }

        //m_messageRead.reset();
        
        // On réécoute
        m_tcpConnection->async_read ( m_messageRead,
                                      boost::bind ( &OlsrEventClient::handleRead, this,
                                                    boost::asio::placeholders::error )
                                    );
    }
    else
    {
        doClose();
    }
}

void OlsrEventClient::doClose()
{
    m_tcpConnection->socket().close();
}

}
}
