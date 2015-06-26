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
#include "/usr/include/netinet/in.h"
#include <iostream>



#include <boost/asio.hpp>
#include <boost/thread.hpp>
//#include <boost/filesystem.hpp>
#include "configfile.h"

#include "olsreventclient.h"
#include "ser_hooks.h"
extern "C" {
#include "common.h"
#include <linux/netfilter.h>        /* for NF_ACCEPT */
#include <linux/netfilter_ipv4.h> // for NF_IP_xxx
#include <libnetfilter_queue/libnetfilter_queue.h>
}

using namespace std;
using namespace sereadmo::olsrclient;
using namespace sereadmo::nfhooks;

// Création du service général
static boost::asio::io_service io_service;

/** Netfilter queue handle */
static struct nfq_handle *h;
static struct nfq_q_handle *qh=0;
static struct nfnl_handle *nh=0;

#define QUEUE_MAXLEN 5000

/* returns packet id */
static u_int32_t print_pkt ( struct nfq_data *tb )
{
    int id = 0;
    struct nfqnl_msg_packet_hdr *ph;
    struct nfqnl_msg_packet_hw *hwph;
    u_int32_t mark,ifi;
    int ret;
    char *data;

    ph = nfq_get_msg_packet_hdr ( tb );
    if ( ph )
    {
        id = ntohl ( ph->packet_id );
        printf ( "hw_protocol=0x%04x hook=%u id=%u ",
                 ntohs ( ph->hw_protocol ), ph->hook, id );
    }

    hwph = nfq_get_packet_hw ( tb );
    if ( hwph )
    {
        int i, hlen = ntohs ( hwph->hw_addrlen );

        printf ( "hw_src_addr=" );
        for ( i = 0; i < hlen-1; i++ )
            printf ( "%02x:", hwph->hw_addr[i] );
        printf ( "%02x ", hwph->hw_addr[hlen-1] );
    }

    mark = nfq_get_nfmark ( tb );
    if ( mark )
        printf ( "mark=%u ", mark );

    ifi = nfq_get_indev ( tb );
    if ( ifi )
        printf ( "indev=%u ", ifi );

    ifi = nfq_get_outdev ( tb );
    if ( ifi )
        printf ( "outdev=%u ", ifi );
    ifi = nfq_get_physindev ( tb );
    if ( ifi )
        printf ( "physindev=%u ", ifi );

    ifi = nfq_get_physoutdev ( tb );
    if ( ifi )
        printf ( "physoutdev=%u ", ifi );

    ret = nfq_get_payload ( tb, &data );
    if ( ret >= 0 )
        printf ( "payload_len=%d ", ret );

    fputc ( '\n', stdout );

    return id;
}


static int nfqueue_cb ( struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
                        struct nfq_data *nfa, void *data )
{
    //u_int32_t id = print_pkt ( nfa );
    int id=0, status=0;
    struct nfqnl_msg_packet_hdr *ph;
    char *payload;
    ph = nfq_get_msg_packet_hdr ( nfa );
    if ( ph )
    {
        id = ntohl ( ph->packet_id );
        switch ( ph->hook )
        {
            case NF_IP_LOCAL_IN:
                //DBG_HK(4,  "NF_LOCAL_IN packet!" );
                //return nfq_set_verdict ( qh, id, NF_ACCEPT, 0, NULL );
                return hk_local_in_hook(qh, nfa);
                break;
            case NF_IP_LOCAL_OUT:
                //DBG_HK(4, "NF_LOCAL_OUT packet!" );
                //return nfq_set_verdict ( qh, id, NF_ACCEPT, 0, NULL );
                return hk_local_out_hook(qh, nfa);
                break;
            case NF_IP_FORWARD:
                DBG_HK(4, "NF_FORWARD packet!" );
                break;
            case NF_IP_PRE_ROUTING:
                hk_pre_routing_hook(qh, nfa);
                break;
            case NF_IP_POST_ROUTING:
                DBG_HK(4, "NF_POST_ROUTING packet!" );
                break;
            default:
                MSG_FAILED( "Not NF_LOCAL_IN/OUT/FORWARD packet!" );
                break;
        }
    }
    else
    {
        MSG_FAILED( "NFQUEUE: can't get msg packet header." );
        return ( 1 );           // from nfqueue source: 0 = ok, >0 = soft error, <0 hard error
    }
    return nfq_set_verdict ( qh, id, NF_ACCEPT, 0, NULL );
}

/**
 * Open a netlink connection and returns file descriptor
 */
int packetsrv_open()
{
    int fd;
    
        DBG_HK(4, "opening library handle" );
        h = nfq_open();
        if ( !h )
        {
            MSG_FAILED("error during nfq_open()" );
            exit ( 1 );
        }

        DBG_HK(4, "unbinding existing nf_queue handler for AF_INET (if any)" );
        if ( nfq_unbind_pf ( h, AF_INET ) < 0 )
        {
            MSG_FAILED("error during nfq_unbind_pf()" );
            exit ( 1 );
        }

        DBG_HK(4, "binding nfnetlink_queue as nf_queue handler for AF_INET" );
        if ( nfq_bind_pf ( h, AF_INET ) < 0 )
        {
            MSG_FAILED("error during nfq_bind_pf()" );
            exit ( 1 );
        }

        DBG_HK(4, "binding this socket to queue '0'" );
        qh = nfq_create_queue ( h,  0, &nfqueue_cb, NULL );
        if ( !qh )
        {
            MSG_FAILED("error during nfq_create_queue()" );
            exit ( 1 );
        }

        DBG_HK(4, "setting copy_packet mode" );
        if ( nfq_set_mode ( qh, NFQNL_COPY_PACKET, 0xffff ) < 0 )
        {
            MSG_FAILED("can't set packet_copy mode" );
            exit ( 1 );
        }

    /* setting queue length */
    if (QUEUE_MAXLEN) {
        if (nfq_set_queue_maxlen(qh, QUEUE_MAXLEN) < 0) {
                MSG_FAILED("[!] Can't set queue length, continuing anyway");
        }
    }

    fd = nfq_fd ( h );

  return fd;
}

//***********************************************************************
//***********************************************************************

void packetsrv_close()
{
    if ( qh )
    {
        DBG_HK(4, "unbinding from queue 0" );
        nfq_destroy_queue ( qh );
    }

#ifdef INSANE
    /* normally, applications SHOULD NOT issue this command, since
     * it detaches other programs/sockets from AF_INET, too ! */
    if (h)
    {
      DBG_HK(4, "unbinding from AF_INET" );
      nfq_unbind_pf ( h, AF_INET );
    }
#endif

    if (h)
    {
      DBG_HK(4, "closing library handle" );
      nfq_close ( h );
    }
}

//***********************************************************************
//***********************************************************************

std::string getBasepath()
{

    std::string path = "";
    pid_t pid = getpid();
    char buf[10];
    sprintf ( buf,"%d",pid );
    std::string _link = "/proc/";
    _link.append ( buf );
    _link.append ( "/exe" );
    char proc[512];
    int ch = readlink ( _link.c_str(),proc,512 );
    if ( ch != -1 )
    {
        proc[ch] = 0;
        path = proc;
        std::string::size_type t = path.find_last_of ( "/" );
        path = path.substr ( 0,t );
    }

    return ( path );
}

//***********************************************************************
//***********************************************************************

void readConfig()
{
  //boost::filesystem::path iniPath = boost::filesystem::initial_path();
  //string strIni = iniPath.native_file_string() + "/ser_routing.conf";
  string strIni = getBasepath()  + "/ser_routing.conf";
  
  ConfigFile::Initialise();
  
  ConfigFile*pConfig = (ConfigFile*)ConfigFile::GetConfigFile( strIni.c_str());
  if (pConfig)
  {
    pConfig->GetIntValue("udp_port", g_conf_udp_port);
    DBG_HK(3,"udp_port=%d", g_conf_udp_port);
    
    // Niveaux de trace
    pConfig->GetIntValue("DebugLevel_nfqueue", g_dbg_hook);
    DBG_HK(3,"DebugLevel_nfqueue=%d", g_dbg_hook);
    pConfig->GetIntValue("DebugLevel_olsrevent", g_dbg_device);
    DBG_HK(3,"DebugLevel_olsrevent=%d", g_dbg_device);
    pConfig->GetIntValue("DebugLevel_dijkstra", g_dbg_djk);
    DBG_HK(3,"DebugLevel_dijkstra=%d", g_dbg_djk);
    
    // k-dijkstra
    pConfig->GetIntValue("nb_path", g_conf_path);
    DBG_HK(3,"Dijkstra_nb_path=%d", g_conf_path);
    
    pConfig->GetIntValue("coef_fe", g_conf_fe);
    DBG_HK(3,"Dijkstra_coef_fe=%d", g_conf_fe);
    
    pConfig->GetIntValue("coef_fp", g_conf_fp);
    DBG_HK(3,"Dijkstra_coef_fp=%d", g_conf_fp);
    
  }
  ConfigFile::Uninitialise();
}

//***********************************************************************
//***********************************************************************

int main ( int argc, char **argv )
{
    int ret=0;
    int fd;
    int rv;
    char buf[4096] __attribute__ ( ( aligned ) );
    //char buf[8192] __attribute__ ( ( aligned ) );
    
    //KCmdLineArgs::init(argc, argv, "ser_routing", "ser_routing", "ser_routing", "1.0");
    //KApplication app;

    readConfig();
    
    try
    {
        // Server IP and connection port
        tcp::endpoint endpoint ( boost::asio::ip::address::from_string ( "127.0.0.1" ), 7171 );

        // Olsr events client creation
        OlsrEventClient c ( io_service );
        if (!c.connect ( endpoint ))
          exit(1);

        // Start main service in a thread
        boost::thread t ( boost::bind ( &boost::asio::io_service::run, &io_service ) );

        fd = packetsrv_open ();
        if (fd < 0) {
          exit(2);
        }
        
        //long i=0;
        while ( true )
        {
          rv = recv ( fd, buf, sizeof ( buf ), 0 );
          if ( rv < 0 )
          {
              MSG_FAILED ("[!] Error of read on netfilter queue socket (code %i)!", rv );
              MSG_FAILED ("Reopen netlink connection." );
              packetsrv_close ();
              fd = packetsrv_open ();
              if ( fd < 0 )
              {
                  MSG_FAILED ("[!] FATAL ERROR: Fail to reopen netlink connection!" );
                  break;
              }
              continue;
          }
      
          //DBG_HK(4, "pkt received %06ld (%d)", i++, rv );
          nfq_handle_packet ( h, buf, rv );
        }
        c.close(); // Ferme la connection
        t.join(); // On rejoint le thread
    }
    catch ( std::exception& e )
    {
      MSG_FAILED("Exception: %s", e.what());
      ret=3;
    }

    packetsrv_close();
    
    exit ( ret );
}
