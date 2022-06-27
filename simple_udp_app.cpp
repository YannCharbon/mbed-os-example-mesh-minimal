#include "simple_udp_app.h"

#include "rtos.h"
#include "mbed-trace/mbed_trace.h"
#include "ip6string.h"
#include "nanostack/socket_api.h"

#include "routing_table_utils.h"

#define MULTICAST_ADDR_STR "ff05::7"
#define HOST_IP			"2a02:1210:741f:d700:dddb:37d3:148c:9c1f"
#define HOST_PORT		42555

char host_addr_str[MAX_IPV6_STRING_LEN_WITH_TRAILING_NULL];

static UDPSocket socket;
//static UDPSocket socket_multicast;
static bool udp_connected = false;

uint8_t multi_cast_addr[16] = {0};
static const int16_t multicast_hops = 20;

DigitalOut led_1(MBED_CONF_APP_LED, 1);
InterruptIn user_button_1(MBED_CONF_APP_BUTTON_1);
InterruptIn user_button_2(MBED_CONF_APP_BUTTON_2);

Thread socket_thread;
Thread socket_multi_thread;
Thread btn_thread;

static bool button_1_pressed = false;
static bool button_2_pressed = false;

char send_buffer[1024];
char recv_buffer[1024];

void app_print(const char *fmt, ...) {
    va_list args;
    printf("\033[0;32m");
    printf("[APP] : ");
    printf("\033[0m");
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void try_connect_to_server();
void receiveUDP();
static void user_button_1_isr();
static void user_button_2_isr();
void button_pressed_handler();

void init_socket(NetworkInterface * interface){
    nsapi_error_t rt = socket.open(interface);
    if (rt != NSAPI_ERROR_OK) {
        app_print("Could not open UDP Socket (%d)\n", rt);
        return;
    }

    socket.set_blocking(false);
    socket.bind(HOST_PORT);
    socket.setsockopt(SOCKET_IPPROTO_IPV6, SOCKET_IPV6_MULTICAST_HOPS, &multicast_hops, sizeof(multicast_hops));

    ns_ipv6_mreq_t mreq;
    stoip6(MULTICAST_ADDR_STR, strlen(MULTICAST_ADDR_STR), multi_cast_addr);
    memcpy(mreq.ipv6mr_multiaddr, multi_cast_addr, 16);
    mreq.ipv6mr_interface = 0;

    socket.setsockopt(SOCKET_IPPROTO_IPV6, SOCKET_IPV6_JOIN_GROUP, &mreq, sizeof mreq);

    socket_thread.start(&receiveUDP);
}


void start_simple_udp_app(NetworkInterface * interface){

    app_print("Starting simple UDP application on port %d\n", HOST_PORT);

#ifdef SIMPLE_UDP_APP_MULTICAST_CONNECT
    app_print("Connection mode : Automatic Multicast\n");
#else
    app_print("Connection mode : Manual IP\n");
#endif  

    if(MBED_CONF_APP_BUTTON_1 != NC){
        app_print("Button 1 is connected to %s\n", MBED_CONF_APP_BUTTON_1);
        user_button_1.fall(&user_button_1_isr);
        user_button_1.mode(MBED_CONF_APP_BUTTON_MODE);
    }
    if(MBED_CONF_APP_BUTTON_2 != NC){
        app_print("Button 2 is connected to %s\n", MBED_CONF_APP_BUTTON_2);
        user_button_2.fall(&user_button_2_isr);
        user_button_2.mode(MBED_CONF_APP_BUTTON_MODE);
    }

    for(int i = 0; i < 10; i++){
        led_1 = !led_1;
        ThisThread::sleep_for(100ms);
    }   

    init_socket(interface);  

    btn_thread.start(&button_pressed_handler);

    try_connect_to_server();
}

void try_connect_to_server(){
    char buffer[] = "CONNECT_REQUEST";
    SocketAddress sock_addr;
    sock_addr.set_port(HOST_PORT);

    while(1){
        if (!udp_connected){       
#ifdef SIMPLE_UDP_APP_MULTICAST_CONNECT
            sock_addr.set_ip_address(MULTICAST_ADDR_STR);
#else
            sock_addr.set_ip_address(HOST_IP);
#endif       
            ThisThread::sleep_for(2s);
        } else {
            sock_addr.set_ip_address(host_addr_str);
            ThisThread::sleep_for(20s);
        } 
        app_print("Sending %s\n", buffer);
        socket.sendto(sock_addr, buffer, strlen(buffer));       
    }
}

// Receive data from the server
void receiveUDP() {
    // Allocate 1K of data for UDP reception
    while (1) {
        // recvfrom blocks until there is data
        SocketAddress source_addr;
        nsapi_size_or_error_t size = socket.recvfrom(&source_addr, recv_buffer, 1024);
        if (size <= 0) {
            if (size == NSAPI_ERROR_WOULD_BLOCK){
                continue; // OK... this is a non-blocking socket and there's no data on the line
            }

            printf("Error while receiving data from UDP socket (%d)\n", size);
            continue;
        }

        // turn into valid C string
        recv_buffer[size] = '\0';
        send_buffer[0] = '\0';

        app_print("Received %s\n", recv_buffer);
        
        if(strcmp(recv_buffer, "CONNECT_RESPONSE") == 0){
            udp_connected = true;
            
            ip6tos(source_addr.get_ip_bytes(), host_addr_str);

            app_print("Connected to UDP server %s\n", host_addr_str);
        } else if(strcmp(recv_buffer, "DISCONNECT_INDICATION") == 0){
            udp_connected = false;

            app_print("Disconnected from UDP server\n");
        } else if (strstr(recv_buffer, "LED_INDICATION") != NULL){
            if(strstr(recv_buffer, "state=1") != NULL){
                app_print("LED ON\n");
                led_1 = 1;
            } else if(strstr(recv_buffer, "state=0") != NULL){
                app_print("LED OFF\n");
                led_1 = 0;
            }
        } else if (strcmp(recv_buffer, "ROUTING_REQUEST") == 0){
            int idx = 0;

            idx += sprintf(send_buffer, "ROUTING_RESPONSE;");

            routing_utils_get_routing_info(&send_buffer[idx], sizeof(send_buffer) - idx);

            SocketAddress sock_addr;
            sock_addr.set_ip_address(host_addr_str);
            sock_addr.set_port(HOST_PORT);
            app_print("Sending : %s\n", send_buffer);
            socket.sendto(sock_addr, send_buffer, strlen(send_buffer));            
        } else if (strcmp(recv_buffer, "BUTTON_INDICATION_MULTI") == 0){
            led_1 = !led_1;
        }
    }
}

static void user_button_1_isr() {
    static bool first_start = true;
    if(first_start){
        first_start = false;
        return;
    }
    button_1_pressed = true;
}

static void user_button_2_isr() {
    static bool first_start = true;
    if(first_start){
        first_start = false;
        return;
    }
    button_2_pressed = true;
}

void button_pressed_handler() {
    while(1){
        if(button_1_pressed){
            char buffer[] = "BUTTON_INDICATION";
            SocketAddress sock_addr;
            sock_addr.set_ip_address(host_addr_str);
            sock_addr.set_port(HOST_PORT);

            app_print("Sending %s\n", buffer);
            socket.sendto(sock_addr, buffer, strlen(buffer));
            button_1_pressed = false;
        }
        if(button_2_pressed){
            char buffer[] = "BUTTON_INDICATION_MULTI";
            SocketAddress sock_addr;
            sock_addr.set_ip_address(MULTICAST_ADDR_STR);
            sock_addr.set_port(HOST_PORT);

            app_print("Sending %s\n", buffer);
            socket.sendto(sock_addr, buffer, strlen(buffer));
            button_2_pressed = false;
        }
        ThisThread::sleep_for(100ms);     
    }
}
