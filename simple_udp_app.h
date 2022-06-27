

#ifndef __SIMPLE_UDP_APP__
#define __SIMPLE_UDP_APP__

#include "mbed.h"
#include "NanostackInterface.h"

#define SIMPLE_UDP_APP_MULTICAST_CONNECT

void start_simple_udp_app(NetworkInterface * interface);

#endif