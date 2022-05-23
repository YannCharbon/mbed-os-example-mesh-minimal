#include "mbed.h"
#include "rtos.h"
#include "nanostack/socket_api.h"
#include "mesh_led_control_example.h"
#include "common_functions.h"
#include "ip6string.h"
#include "mbed-trace/mbed_trace.h"
#include "UDPSocket.h"
#include "LWIPStack.h"


// Time protocol implementation : Address: time.nist.gov UDPPort: 37  
 
typedef struct {
    uint32_t secs;         // Transmit Time-stamp seconds.
}ntp_packet;
 
int start_mesh_udp_client_example(NetworkInterface * net) {
    // Bring up the ethernet interface
    printf("UDP Socket example\n");
 
    UDPSocket* sock = new UDPSocket();
    sock->open(net);
    sock->set_blocking(false);

    SocketAddress sockAddr;

    while(1){
        ThisThread::sleep_for(5s);
        
        char out_buffer[] = "time";
        if(0 > sock->sendto("time.nist.gov", out_buffer, sizeof(out_buffer))) {
            printf("Error sending data\n");
            continue;
        }
        
        ntp_packet in_data;
        int n = sock->recvfrom(&sockAddr, &in_data, sizeof(ntp_packet));
        in_data.secs = ntohl( in_data.secs ) - 2208988800;    // 1900-1970
        printf("Time Received %lu seconds since 1/01/1900 00:00 GMT\n", 
                            (uint32_t)in_data.secs);
        printf("Time = %s", ctime(( const time_t* )&in_data.secs));
        
        printf("Time Server Address: %s Port: %d\n\r", 
                                sockAddr.get_ip_address(), sockAddr.get_port());
        
    }
}