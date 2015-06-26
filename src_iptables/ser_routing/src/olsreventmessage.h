#ifndef OLSR_EVENT_MSG_H
#define OLSR_EVENT_MSG_H

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>

namespace sereadmo {
namespace olsrclient {

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
    DEV_CMD_SET_ALIAS_ADDR = 9,
    DEV_CMD_SET_DEBUG_LEVEL = 10
  };
};
}
}
#endif
