#include "simple_udp_app.h"

#include "rtos.h"
#include "mbed-trace/mbed_trace.h"
#include "ip6string.h"
#include "nanostack/socket_api.h"

#define multicast_addr_str "ff15::810a:64d1"
//#define HOST_IP			"2a02:1210:741f:d700:ad75:e110:b0a2:cb3"
//#define HOST_IP         "2a02:1210:741f:d700:452a:6e9f:9827:dca0"
#define HOST_IP			"2a02:1210:741f:d700:a1ab:58ea:8fc5:4e23"
#define HOST_PORT		42555

static UDPSocket socket;
static bool udp_connected = false;

DigitalOut led_1(MBED_CONF_APP_LED, 1);
InterruptIn user_button(MBED_CONF_APP_BUTTON);

static bool button_pressed = false;

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
static void user_button_isr();
void button_pressed_handler();


void start_simple_udp_app(NetworkInterface * interface){

    app_print("Starting simple UDP application on port %d\n", HOST_PORT);

    user_button.fall(&user_button_isr);
    user_button.mode(MBED_CONF_APP_BUTTON_MODE);

    for(int i = 0; i < 10; i++){
        led_1 = !led_1;
        ThisThread::sleep_for(100ms);
    }    

    // Open a UDP socket
    nsapi_error_t rt = socket.open(interface);
    if (rt != NSAPI_ERROR_OK) {
        app_print("Could not open UDP Socket (%d)\n", rt);
        return;
    }

    socket.set_blocking(false);

    Thread socket_thread;
    socket_thread.start(&receiveUDP);

    Thread btn_thread;
    btn_thread.start(&button_pressed_handler);

    try_connect_to_server();
}

void try_connect_to_server(){
    char buffer[] = "CONNECT_REQUEST";
    SocketAddress sock_addr;
    sock_addr.set_ip_address(HOST_IP);
    sock_addr.set_port(HOST_PORT);

    while(1){
        app_print("Sending %s\n", buffer);
        socket.sendto(sock_addr, buffer, strlen(buffer));
        if (!udp_connected){            
            ThisThread::sleep_for(2s);
        } else {
            ThisThread::sleep_for(20s);
        }       
        
    }
}

// Receive data from the server
void receiveUDP() {
    // Allocate 2K of data
    char* data = (char*)malloc(2048);
    while (1) {
        // recvfrom blocks until there is data
        SocketAddress source_addr;
        nsapi_size_or_error_t size = socket.recvfrom(&source_addr, data, 2048);
        if (size <= 0) {
            if (size == NSAPI_ERROR_WOULD_BLOCK){
                continue; // OK... this is a non-blocking socket and there's no data on the line
            }

            printf("Error while receiving data from UDP socket (%d)\n", size);
            continue;
        }

        // turn into valid C string
        data[size] = '\0';

        app_print("Received %s\n", data);
        
        if(strcmp(data, "CONNECT_RESPONSE") == 0){
            udp_connected = true;

            char src_ip_addr_str[MAX_IPV6_STRING_LEN_WITH_TRAILING_NULL];
            ip6tos(source_addr.get_ip_bytes(), src_ip_addr_str);

            app_print("Connected to UDP server %s\n", src_ip_addr_str);
        } else if(strcmp(data, "DISCONNECT_INDICATION") == 0){
            udp_connected = false;

            app_print("Disconnected from UDP server\n");
        } else if (strstr(data, "LED_INDICATION") != NULL){
            if(strstr(data, "state=1") != NULL){
                app_print("LED ON\n");
                led_1 = 1;
            } else if(strstr(data, "state=0") != NULL){
                app_print("LED OFF\n");
                led_1 = 0;
            }
        }
    }
}

static void user_button_isr() {
    button_pressed = true;
}

void button_pressed_handler() {
    char buffer[] = "BUTTON_INDICATION";
    SocketAddress sock_addr;
    sock_addr.set_ip_address(HOST_IP);
    sock_addr.set_port(HOST_PORT);

    while(1){
        if(button_pressed){
            app_print("Sending %s\n", buffer);
            socket.sendto(sock_addr, buffer, strlen(buffer));
            button_pressed = false;
        }   
        ThisThread::sleep_for(100ms);     
    }
}
