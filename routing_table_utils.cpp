#include "routing_table_utils.h"

#include "mbed-os/connectivity/libraries/nanostack-libservice/mbed-client-libservice/ns_list.h"
#include "mbed-os/connectivity/nanostack/sal-stack-nanostack/nanostack/net_interface.h"

#include "nsconfig.h"
#include "ns_types.h"
#include "common_functions.h"
#include "ip6string.h"
#include "string.h"
#include "ipv6_stack/ipv6_routing_table.h"
#include "Common_Protocols/ipv6_constants.h"
#include <stdio.h>

void routing_utils_print_next_hops(){
    char str[1024] = { '\0' };
    arm_get_routing_table_as_string(str, sizeof(str));

    printf("%s\n", str);
}

void routing_utils_print_rpl_instance(){
    char str[1024] = { '\0' };
    arm_rpl_control_get_dodag_as_string(str, sizeof(str));

    printf("%s\n", str);
}

int routing_utils_get_routing_info(char* str, int len){
    int idx = 0;

    idx += arm_get_routing_table_as_string(&str[idx], len - idx);

    idx += arm_rpl_control_get_dodag_as_string(&str[idx], len - idx);

    return idx;
}