#ifndef RAW_SOCKET_H
#define RAW_SOCKET_H

#include "common.h"
namespace sereadmo {
namespace net {

class RawSocket
{
public:
	/*
	 * RawSocket():
	 *	prepare the raw socket and return the handle (file descriptor)
	 */
	RawSocket();
	virtual ~RawSocket(); 
	
	virtual bool close();

	/*
	 * send(buf, len, flags):
	 *	write the packet from buf into fd, return bytes actually written
	 */
	int sendTo(ip_v4_addr daddr, unsigned char *buf, int len, int flags=0);

private: 
    int sock_;

};

}
}
#endif


