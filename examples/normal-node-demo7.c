#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>
#include <setjmp.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <ndn-lite.h>
#include "ndn-lite.h"
#include "ndn-lite/encode/name.h"
#include "ndn-lite/encode/data.h"
#include "ndn-lite/encode/interest.h"
#include "ndn-lite/encode/signed-interest.h"
#include "ndn-lite/app-support/service-discovery.h"
#include "ndn-lite/app-support/access-control.h"
#include "ndn-lite/app-support/security-bootstrapping.h"
#include "ndn-lite/app-support/ndn-sig-verifier.h"
#include "ndn-lite/app-support/pub-sub.h"
#include "ndn-lite/encode/key-storage.h"
#include "ndn-lite/encode/ndn-rule-storage.h"
#include "ndn-lite/forwarder/pit.h"
#include "ndn-lite/forwarder/fib.h"
#include "ndn-lite/forwarder/forwarder.h"
#include "ndn-lite/util/uniform-time.h"
#include "ndn-lite/forwarder/face.h"

#define PORT 8888
#define NODE1 "10.156.82.252"
#define NODE2 "10.156.82.254"
#define NODE3 "10.156.82.255"
#define NODE4 "10.156.83.0"
#define NODE5 "10.156.83.1"
#define NODE6 "10.156.83.2"
#define NODE7 "10.156.83.3"
#define NODE8 "10.156.83.5"
#define NODE9 "10.156.83.4"
#define NODE10 "10.156.83.6"
#define NODE11 "10.156.83.7"
#define NODE12 "10.156.83.8"
#define NODE13 "10.156.82.253"
#define NODE14 "10.156.83.9"
#define DEBUG "10.156.66.137"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

//backslash used for multiline macros
//question mark is ternary operator ex: if (byte & 0x80), then 1 else 0
//checks every bit, if bit in "byte" is 1, the "1", else "0"

//in the build directory go to make files and normal node -change the link.txt
// CMAKE again
// then make
// link.txt
// /usr/bin/cc  -std=c11 -Werror -Wno-format -Wno-int-to-void-pointer-cast -Wno-int-to-pointer-cast -O3   CMakeFiles/normal-node.dir/examples/normal-node.c.o  -pthread -lm -o examples/normal-node  libndn-lite.a

// sizeof returns the size of a the type if getting size of pointer, if size of an array, then it prints out length of an array
//add newlines to print statements for correct debug of seg fault to match gdb

ndn_udp_face_t *generate_udp_face(char* input_ip, char *port_1, char *port_2);

typedef struct anchor_pit_entry {
    ndn_name_t name_struct;
    char *prefix;
} anchor_pit_entry_t;

//for linking prefixes to a specific face
typedef struct anchor_pit {
    //change size to be more dynamic when iterating through array
    anchor_pit_entry_t slots[40];
} anchor_pit_t;

typedef struct ip_table_entry {
    char *ip_address;
    char *node_num;
} ip_table_entry_t;

typedef struct ip_table {
    int last_entry;
    ip_table_entry_t entries[40];
} ip_table_t;

typedef struct udp_face_table_entry {
    ndn_udp_face_t *face_entry;
    bool is_filled;
} udp_face_table_entry_t;

typedef struct udp_face_table {
    udp_face_table_entry_t entries[40];
} udp_face_table_t;

//-----------------------------

typedef struct node_data1_index {
    uint8_t index[2];
    //outgoing_array will never have a valid 0 due to there being non-zero node num only
    int outgoing_array[20];
    bool is_filled;
} node_data1_index_t;

typedef struct content_store_entry {
    ndn_data_t data_pkt;
    uint8_t vector_num[5];
    //data1_array[0] = anchor 1
    node_data1_index_t data1_array[40];
    bool is_filled;
} content_store_entry_t;

//-----------------------------

typedef struct anchor_data1_index {
    uint8_t data_value[1024];
    bool is_filled;
} anchor_data1_index_t;

//-----------------------------

typedef struct content_store {
    content_store_entry_t entries[4096];
    anchor_data1_index_t data_indexes[4096];
} content_store_t;

typedef struct delay_struct {
    int struct_selector;
    ndn_interest_t interest;
} delay_struct_t;

ip_table_t ip_list;
anchor_pit_t node_anchor_pit;
content_store_t cs_table;
udp_face_table_t udp_table;

//To start/stop main loop
bool running;

//boolean to check if node is connected to debug server
bool debug_connected;
bool debug_connected_graph;

//To set whether this node is anchor
bool is_anchor = false;

//Time Slice
int time_slice = 3;

//Selector integers
//selector will be set from hash function of previous block
//uint8_t selector[10] = {1,2,3,4,5,6,7,8,9,10}; //change from 0 to 9
//uint8_t *selector_ptr = selector;


// should scale with number of nodes in network: stored_selectors, delay_start, interface_num, did_flood
bool stored_selectors[40];

bool delay_start[40];
//set array for multiple anchors for anchor/selector 1 - 10
int interface_num[40];
//for telling if a node has flooded for the specific anchor
bool did_flood[40];

//clock time is in nano seconds, divide by 10^6 for actual time
int delay = 3000000;
int max_interfaces = 2;

// time of last interest for error checking
int last_interest;

//ndn keys
ndn_ecc_prv_t *ecc_secp256r1_prv_key;
ndn_ecc_pub_t *ecc_secp256r1_pub_key;
//ndn_key_storage_t *storage;

//Signature data for node (private key)
uint8_t secp256r1_prv_key_str[32] = {
0xA7, 0x58, 0x4C, 0xAB, 0xD3, 0x82, 0x82, 0x5B, 0x38, 0x9F, 0xA5, 0x45, 0x73, 0x00, 0x0A, 0x32,
0x42, 0x7C, 0x12, 0x2F, 0x42, 0x4D, 0xB2, 0xAD, 0x49, 0x8C, 0x8D, 0xBF, 0x80, 0xC9, 0x36, 0xB5
};

//Signature data for node (puplic key)
uint8_t secp256r1_pub_key_str[64] = {
0x99, 0x26, 0xD6, 0xCE, 0xF8, 0x39, 0x0A, 0x05, 0xD1, 0x8C, 0x10, 0xAE, 0xEF, 0x3C, 0x2A, 0x3C,
0x56, 0x06, 0xC4, 0x46, 0x0C, 0xE9, 0xE5, 0xE7, 0xE6, 0x04, 0x26, 0x43, 0x13, 0x8A, 0x3E, 0xD4,
0x6E, 0xBE, 0x0F, 0xD2, 0xA2, 0x05, 0x0F, 0x00, 0xAC, 0x6F, 0x5D, 0x4B, 0x29, 0x77, 0x2D, 0x54,
0x32, 0x27, 0xDC, 0x05, 0x77, 0xA7, 0xDC, 0xE0, 0xA2, 0x69, 0xC8, 0x8B, 0x4C, 0xBF, 0x25, 0xF2
};

//socket variables
int sock = 0;
struct sockaddr_in serv_addr;

int sock_graph = 0;
struct sockaddr_in serv_addr_graph;

//ndn_udp_face_t *face1, *face2, *face3, *face4, *face5, *face6, *face7, *face8, *face9, *face10, *data_face;

//for fill pit to see if max interfaces has been reached for that anchor
int ancmt_num[40];

//node_num future use for the third slot in prefix
int node_num = 1;

//list of neighbor selectors to current node
int neighbor_list[20];

//list of neighbor nodes that will be flooded to from current node (ancmt only)
int flood_list[20];

int send_debug_message(char *input) {
    if(debug_connected == true) {
        char *debug_message;
        //char buffer[1024] = {0};
        //int valread;
        debug_message = input;
        send(sock , debug_message, strlen(debug_message) , 0 );

        //printf("Hello message sent\n");
        //valread = read( sock , buffer, 1024);
        //printf("%s\n",buffer);
    }
    if(debug_connected_graph == true) {
        char *debug_message;
        debug_message = input;
        send(sock_graph , debug_message, strlen(debug_message) , 0 );
    }
    return 0;
}

char *timestamp() {
    struct timeval tv;
    struct timezone tz;
    struct tm *today;
    int zone;

    //gettimeofday(&tv,&tz);
    gettimeofday(&tv, &tz);
    time_t timer = tv.tv_sec;
    today = localtime(&timer);
    //printf("TIME: %d:%0d:%0d.%d\n", today->tm_hour, today->tm_min, today->tm_sec, tv.tv_usec);

    //we set index 0 to be 0 to init
    char *return_string;
    return_string = malloc(40); 
    return_string[0] = 0;

    sprintf(return_string, "%d.%0d.%0d.%d", today->tm_hour, today->tm_min, today->tm_sec, tv.tv_usec);
    return return_string;
}

//used to add to neighbor_list
void add_neighbor(int neighbor_num) {
    size_t nl_size = sizeof(neighbor_list)/sizeof(neighbor_list[0]);
    for(size_t i = 0; i < nl_size; i++) {
        if(neighbor_list[i] == 0) {
            neighbor_list[i] = neighbor_num;
            return;
        }
    }
}

//adds IP constants to table for search_ip_table
void add_ip_table(char *input_num, char *input_ip) {
    int num;
    num = atoi(input_num);
    //entries[0] = Node 1
    ip_list.entries[num-1].node_num = input_num;
    ip_list.entries[num-1].ip_address = input_ip;
}

//used to find ip linked to node number
char *search_ip_table(int input_num) {
    printf("Search IP Table\n");
    char *return_var = "";
    printf("SEARCH INDEX: %d\n", input_num);
    //does not have to search through arrat of ip_list.entries, index of array = node_num-1
    return_var = ip_list.entries[input_num-1].ip_address;
    printf("RETURN IP: %s\n", return_var);
    return return_var;
}

//not used
//inet ntoa
char *get_ip_address_string(ndn_udp_face_t *input_face) {
    char *output = "";
    struct in_addr input;
    input = input_face->remote_addr.sin_addr;
    output = inet_ntoa(input);
    return output;
}

//gets first slot
char *get_string_prefix(ndn_name_t input_name) {
    char *return_string;
    return_string = malloc(40); 
    return_string[0] = 0;
    ndn_name_t prefix_name;
    prefix_name = input_name;

    for (int i = 0; i < prefix_name.components_size; i++) {
        //printf("%d, ",prefix_name.components[i].type);
        if(prefix_name.components[i].type == 8) {
            strcat(return_string,"/");
            for (int j = 0; j < prefix_name.components[i].size; j++) {
                if (prefix_name.components[i].value[j] >= 33 && prefix_name.components[i].value[j] < 126) {
                    char temp_char[10];
                    sprintf(temp_char, "%c", prefix_name.components[i].value[j]);
                    strcat(return_string, temp_char);
                }
                // else {
                //     printf("0x%02x", component.value[j]);
                // }
            }
        }

    }
    return return_string;
}

//gets prefix slot of input
char *get_prefix_component(ndn_name_t input_name, int num_input) {
    //printf("Get Prefix Component %d\n",num_input);
    char *return_string;
    return_string = malloc(40); 
    return_string[0] = 0;
    ndn_name_t prefix_name;
    prefix_name = input_name;

    //printf("%d, ",prefix_name.components[i].type);
    if(prefix_name.components[num_input].type == 8) {
        for (int j = 0; j < prefix_name.components[num_input].size; j++) {
            if (prefix_name.components[num_input].value[j] >= 33 && prefix_name.components[num_input].value[j] < 126) {
                char temp_char[10];
                sprintf(temp_char, "%c", prefix_name.components[num_input].value[j]);
                strcat(return_string, temp_char);
            }
        }
    }

    return return_string;
}

//may have to use interest as a pointer
//flood has to include the anchor prefix for second slot in ancmt packet
void flood(ndn_interest_t interest_pkt, char *second_slot) {
    printf("\nFlooding\n");
    ndn_interest_t interest;
    ndn_name_t prefix_name;
    ndn_udp_face_t *face;

    ndn_encoder_t encoder;
    uint8_t encoding_buf[2048];

    char change_num[20] = "";
    sprintf(change_num, "%d", node_num);
    //change this to available ancmt prefix
    // char ancmt_string[20] = "/ancmt/1/";
    // strcat(ancmt_string, change_num);
    // ndn_name_from_string(&prefix_name, ancmt_string, strlen(ancmt_string));
    char ancmt_string[20] = "/ancmt/";

    //if(is_anchor == true) {
    int second_slot_num;
    second_slot_num = atoi(second_slot);
    bool route_added = false;

    if(second_slot_num == node_num) {
        printf("Anchor Flood\n");
        strcat(ancmt_string, change_num);
        strcat(ancmt_string, "/");
        strcat(ancmt_string, change_num);
        
        ndn_name_from_string(&prefix_name, ancmt_string, strlen(ancmt_string));
        ndn_interest_from_name(&interest, &prefix_name);
        encoder_init(&encoder, encoding_buf, sizeof(encoding_buf));
        ndn_interest_tlv_encode(&encoder, &interest);
        
        size_t nl_size = sizeof(neighbor_list)/sizeof(neighbor_list[0]);
        for(size_t i = 0; i < nl_size; i++) {
            if(neighbor_list[i] != 0) {
                char *ip_string = "";
                ip_string = search_ip_table(neighbor_list[i]);
                face = generate_udp_face(ip_string, "3000", "5000");
                // ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);
                ndn_face_send(&face->intf, encoder.output_value, encoder.offset);
                route_added = true;
            }
        }
    }

    else {
        printf("Normal Flood\n");
        strcat(ancmt_string, second_slot);
        strcat(ancmt_string, "/");
        strcat(ancmt_string, change_num);

        ndn_name_from_string(&prefix_name, ancmt_string, strlen(ancmt_string));
        ndn_interest_from_name(&interest, &prefix_name);
        encoder_init(&encoder, encoding_buf, sizeof(encoding_buf));
        ndn_interest_tlv_encode(&encoder, &interest);

        int received_ancmts[10];
        int next_index = 0;

        size_t nap_size = sizeof(node_anchor_pit.slots)/sizeof(node_anchor_pit.slots[0]);
        for(size_t i = 0; i < nap_size; i++) {
            char *check_ancmt = "";
            check_ancmt = get_prefix_component(node_anchor_pit.slots[i].name_struct, 0);
            char *second_prefix = "";
            second_prefix = get_prefix_component(node_anchor_pit.slots[i].name_struct, 1);
            //if pit prefix is ancmt and the entry has the same anchor selector as the function that is calling flood then add to recieved ancmts to flood
            if(strcmp(check_ancmt, "ancmt") == 0 && strcmp(second_prefix, second_slot) == 0) {
                received_ancmts[next_index] = atoi(get_prefix_component(node_anchor_pit.slots[i].name_struct, 2));
                next_index++;
            }
        }

        //this is for flooding to all nodes that are
        size_t nl_size = sizeof(neighbor_list)/sizeof(neighbor_list[0]);
        size_t ra_size = sizeof(received_ancmts)/sizeof(received_ancmts[0]);
        for(size_t i = 0; i < nl_size; i++) {
            bool do_skip = false;
            if(neighbor_list[i] == 0) {
                do_skip = true;
            }
            
            else {
                for(size_t j = 0; j < ra_size; j++) {
                    //Note: segmentation fault here if both neighbor_list[i] = 0 and received_ancmts[j] = 0, seacch ip_table of index 0
                    if(neighbor_list[i] == received_ancmts[j]) {
                        do_skip = true;
                    }
                }
            }
            
            if(do_skip == false) {
                char *ip_string = "";
                ip_string = search_ip_table(neighbor_list[i]);
                //TODO: change here to have an array of faces that all send at close to the same time
                face = generate_udp_face(ip_string, "3000", "5000");
                // ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);
                ndn_face_send(&face->intf, encoder.output_value, encoder.offset);
                route_added = true;
            }
        }
    }

    // face = generate_udp_face(NODE2, "3000", "5000");
    // ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);

    // face = generate_udp_face(NODE3, "3000", "5000");
    // ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);

    if(route_added == true) {
        printf("Flooded Interest!\n");
        // ndn_interest_from_name(&interest, &prefix_name);
        // ndn_forwarder_express_interest_struct(&interest, NULL, NULL, NULL);
    }
    else {
        printf("Function Complete Without Sending\n");
    }
    // send_debug_message("Flooded Interest ; ");
}

// void send_ancmt() {
//     printf("\nSending Announcement...\n");

//     //include periodic subscribe of send_anct
//     //ndn_interest_t *ancmt = new ndn_interest_t();
//     //malloc
//     ndn_interest_t ancmt;
//     ndn_encoder_t encoder;
//     ndn_udp_face_t *face;
//     ndn_name_t prefix_name;
//     char *prefix_string = "/ancmt/1/1";
//     char interest_buf[4096];

//     //. instead ->, initialize as a pointer object first, testing new keyword

//     //Sets timestamp
//     //time_t clk = time(NULL);
//     //char* timestamp = ctime(&clk);

//     //This creates the routes for the interest and sends to nodes
//     //ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);
//     ndn_name_from_string(&prefix_name, prefix_string, strlen(prefix_string));
//     ndn_interest_from_name(&ancmt, &prefix_name);
//     //ndn_forwarder_express_interest_struct(&interest, on_data, on_timeout, NULL);

//     //gets ndn (timestamp)
//     ndn_time_ms_t timestamp = ndn_time_now_ms();

//     //parameter may be one whole string so the parameters may have to be sorted and stored in a way that is readabel by other normal nodes
//     //Init ancmt with selector, signature, and timestamp
//     //may have to use ex: (uint8_t*)str for middle param
    
//     //ndn_interest_set_Parameters(&ancmt, (uint8_t*)&timestamp, sizeof(timestamp));
//     //ndn_interest_set_Parameters(&ancmt, (uint8_t*)(selector_ptr + 1), sizeof(selector[1]));
//     //ndn_interest_set_Parameters(&ancmt, (uint8_t*)ip_address, sizeof(ip_address));

//     //Signed interest init
//     ndn_key_storage_get_empty_ecc_key(&ecc_secp256r1_pub_key, &ecc_secp256r1_prv_key);
//     ndn_ecc_make_key(ecc_secp256r1_pub_key, ecc_secp256r1_prv_key, NDN_ECDSA_CURVE_SECP256R1, 890);
//     ndn_ecc_prv_init(ecc_secp256r1_prv_key, secp256r1_prv_key_str, sizeof(secp256r1_prv_key_str), NDN_ECDSA_CURVE_SECP256R1, 0);
//     ndn_key_storage_t *storage = ndn_key_storage_get_instance();
//     ndn_name_t *self_identity_ptr = storage->self_identity;
//     ndn_signed_interest_ecdsa_sign(&ancmt, self_identity_ptr, ecc_secp256r1_prv_key);
//     // encoder_init(&encoder, interest_buf, 4096);
//     // ndn_interest_tlv_encode(&encoder, &ancmt);

//     //uncomment here to test flood
//     flood(ancmt);
//     printf("Announcement sent.\n");
//     //send_debug_message("Announcment Sent");
// }

//check signature is correct from the public key is valid for all normal nodes
//check if timestamp is before the current time
bool verify_interest(ndn_interest_t *interest) {
    printf("\nVerifying Packet\n");
    //int timestamp = interest->signature.timestamp;
    int timestamp = 0;
    int current_time = ndn_time_now_ms();
    //verify time slot

    if((current_time - timestamp) < 0) {
        return false;
    }
    else if(ndn_signed_interest_ecdsa_verify(interest, ecc_secp256r1_pub_key) != NDN_SUCCESS) {
        return false;
    }
    //send_debug_message("Interest Verified");
    return true;
}

//reply ancmt not used by anchor nodes so add if statement in flood statements to account for this
//only reply acnmt if anchor when receiveing from another anchor in the network
void reply_ancmt(char *second_slot) {
    //send_debug_message("Announcent Reply Sent");
    printf("\nReply Ancmt...\n");
    int reply[10];
    int counter = 0;

    ndn_encoder_t encoder;
    uint8_t encoding_buf[2048];

    //TODO: change all code when we iterate through pit to find ancmts
    //change to only send ancmts from second_slot input
    size_t nap_size = sizeof(node_anchor_pit.slots)/sizeof(node_anchor_pit.slots[0]);
    for(size_t i = 0; i < nap_size; i++) {
        char *check_ancmt = "";
        check_ancmt = get_prefix_component(node_anchor_pit.slots[i].name_struct, 0);
        char *check_ancmt_anchor = "";
        check_ancmt_anchor =  get_prefix_component(node_anchor_pit.slots[i].name_struct, 1);
        if(strcmp(check_ancmt, "ancmt") == 0 && atoi(check_ancmt_anchor) == atoi(second_slot)) {
            reply[counter] = atoi(get_prefix_component(node_anchor_pit.slots[i].name_struct, 2));
            counter++;
        }
    }

    srand(time(0));
    int rand_num = rand() % counter;

    char *ip_string = "";
    ip_string = search_ip_table(reply[rand_num]);
    printf("LOOKUP IP: %s\n", ip_string);
    
    ndn_interest_t interest;
    ndn_name_t prefix_name;
    ndn_udp_face_t *face;

    char change_num[20] = "";
    sprintf(change_num, "%d", node_num);
    char ancmt_string[20] = "/l2interest/";
    strcat(ancmt_string, second_slot);
    strcat(ancmt_string, "/");
    strcat(ancmt_string, change_num);

    ndn_name_from_string(&prefix_name, ancmt_string, strlen(ancmt_string));
    ndn_interest_from_name(&interest, &prefix_name);
    encoder_init(&encoder, encoding_buf, sizeof(encoding_buf));
    ndn_interest_tlv_encode(&encoder, &interest);

    face = generate_udp_face(ip_string, "4000", "6000");
    ndn_face_send(&face->intf, encoder.output_value, encoder.offset);

    // ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);
    // ndn_interest_from_name(&interest, &prefix_name);
    // ndn_forwarder_express_interest_struct(&interest, NULL, NULL, NULL);
}

//input is name
void generate_layer_2_data(char *input_ip, char *second_slot, uint8_t *data_string, uint32_t data_size) {
    printf("\nGenerate Layer 2 Data\n");
    ndn_data_t data;
    ndn_name_t prefix_name;
    ndn_udp_face_t *face;
    ndn_encoder_t encoder;
    uint8_t buf[4096];

    //layer 2 data_string: /bit_vector(5)/data_index_new(2)/data content
    //vector: /bit_vector(5)/anchor_num_old(2)/data_index_old(2)/data_index_new(2)/ and then associate data_index_new with the second slot anchor prefix for current cs
    //bit vector should include its own anchor node
    uint8_t *str = data_string;

    //prefix string can be anything here because data_recieve bypasses prefix check in fwd_data_pipeline
    char change_num[20] = "";
    sprintf(change_num, "%d", node_num);
    char prefix_string[40] = "/l2data/";
    strcat(prefix_string, second_slot);
    strcat(prefix_string, "/");
    strcat(prefix_string, change_num);
    ndn_name_from_string(&prefix_name, prefix_string, strlen(prefix_string));

    data.name = prefix_name;
    //ndn_data_set_content(&data, (uint8_t*)str, strlen(str) + 1);
    //cant use sizeof with pointer of char, must use strlen + 1 to account for null char at end of string
    //no need to add plus 1 again because generate_data already added 1 for null character so no need to add on again
    //0x00 is a null character denoting the end of a string in c
    ndn_data_set_content(&data, str, data_size + 7);
    ndn_metainfo_init(&data.metainfo);
    ndn_metainfo_set_content_type(&data.metainfo, NDN_CONTENT_TYPE_BLOB);
    encoder_init(&encoder, buf, 4096);
    ndn_data_tlv_encode_digest_sign(&encoder, &data);

    face = generate_udp_face(input_ip, "6000", "4000");
    ndn_face_send(&face->intf, encoder.output_value, encoder.offset);

    send_debug_message("Layer 2 Data Sent ; ");
}

//sends data anchor direction (layer1)
//using different port because dont know if prefix name will interfere with ndn_forwarder for sending data
//actually this used the 5000 3000 interface to send data(this is along the same face as ancmt)
//prefix: /l1data/anchor_num/sender_num
void generate_data(char *data_string) {
    printf("\nGenerate Layer 1 Data: %s\n", data_string);
    ndn_data_t data;
    ndn_name_t prefix_name;
    ndn_udp_face_t *face;
    ndn_encoder_t encoder;
    uint8_t buf[4096];
    char *str = data_string;

    //iterate through all anchors that sent ancmts
    for(size_t j = 0; j < sizeof(ancmt_num)/sizeof(ancmt_num[0]); j++) {
        if(ancmt_num[j] != 0) {

            int reply[10];
            int counter = 0;
            //prefix string can be anything here because data_recieve bypasses prefix check in fwd_data_pipeline

            //TODO: need to distinguish ancmts inside reply to send # of replies based on number of anchors)
            size_t nap_size = sizeof(node_anchor_pit.slots)/sizeof(node_anchor_pit.slots[0]);
            for(size_t i = 0; i < nap_size; i++) {
                char *check_ancmt = "";
                check_ancmt = get_prefix_component(node_anchor_pit.slots[i].name_struct, 0);
                char *check_ancmt_anchor = "";
                check_ancmt_anchor =  get_prefix_component(node_anchor_pit.slots[i].name_struct, 1);
                if(strcmp(check_ancmt, "ancmt") == 0 && atoi(check_ancmt_anchor) == (j+1)) {
                    reply[counter] = atoi(get_prefix_component(node_anchor_pit.slots[i].name_struct, 2));
                    counter++;
                }
            }

            char local_num[40] = "";
            sprintf(local_num, "%d", node_num);
            char dest_num[40] = "";
            sprintf(dest_num, "%d", (j+1));
            char prefix_string[20] = "/l1data/";
            strcat(prefix_string, dest_num);
            strcat(prefix_string, "/");
            strcat(prefix_string, local_num);
            ndn_name_from_string(&prefix_name, prefix_string, strlen(prefix_string));

            srand(time(0));
            int rand_num = rand() % counter;

            char *ip_string;
            ip_string = search_ip_table(reply[rand_num]);

            data.name = prefix_name;
            ndn_data_set_content(&data, (uint8_t*)str, strlen(str) + 1);
            ndn_metainfo_init(&data.metainfo);
            ndn_metainfo_set_content_type(&data.metainfo, NDN_CONTENT_TYPE_BLOB);
            encoder_init(&encoder, buf, 4096);
            ndn_data_tlv_encode_digest_sign(&encoder, &data);

            face = generate_udp_face(ip_string, "5000", "3000");
            ndn_face_send(&face->intf, encoder.output_value, encoder.offset);

        }
    }

    char *in = "";
    in = timestamp();

    char pub_message[100] = "";
    strcat(pub_message, "1:");
    strcat(pub_message, in);
    strcat(pub_message, ">");
    strcat(pub_message, str);
    strcat(pub_message, "_");
    char tp_size[10] = "";
    sprintf(tp_size, "%d", encoder.offset);
    strcat(pub_message, tp_size);
    strcat(pub_message, " ; ");
    send_debug_message(pub_message);

    // send_debug_message("Layer 1 Data Sent ; ");
}

void *start_delay(void *arguments) {
    printf("\nDelay started\n");
    delay_struct_t *args = arguments;

    //starts delay and adds onto max interfaces
    clock_t start_time = clock();
    printf("Delay Time: %d seconds\n", delay/1000000);
    while (clock() < start_time + delay) {
    }
    printf("Delay Complete\n");
    //then when finished, flood
    if(did_flood[args->struct_selector] == true) {
        printf("Already flooded\n");
    } 
    else {
        //fix this with new flood function
        flood(args->interest, get_prefix_component(args->interest.name, 1));
        did_flood[args->struct_selector] = true;
        // if(is_anchor == false) {
        if(atoi(get_prefix_component(args->interest.name, 1)) != node_num) {
            reply_ancmt(get_prefix_component(args->interest.name, 1));
        }
        //pthread_exit(NULL);
    }
}

char *trimwhitespace(char *str) {
    char *end;

    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)
    return str;

    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    end[1] = '\0';

    return str;
}

//is this threaded, if so, then
//non zero chance of flooding twice due to multithreading
int on_interest(const uint8_t* interest, uint32_t interest_size, void* userdata) {
    printf("\nNormal-Node On Interest\n");

    //is prefix ancmt or l2interest

    pthread_t layer1;
    //pthread_t per_pub;
    ndn_interest_t interest_pkt;
    ndn_interest_from_block(&interest_pkt, interest, interest_size);

    char *prefix = "";
    prefix = get_string_prefix(interest_pkt.name);
    printf("PREFIX: %s\n", prefix);
    printf("ON INTEREST LENGTH: %d\n", interest_size);

    //TODO: make this a function later
    //strcat requires an array of dedicated size
    //should be third slot in prefix
    // prefix = get_prefix_component(interest_pkt.name, 2);
    // prefix = trimwhitespace(prefix);
    // prefix = get_string_prefix(interest_pkt.name);
    char temp_message[80] = "";
    strcat(temp_message, "On Interest: ");
    strcat(temp_message, prefix);
    strcat(temp_message, " ; ");
    send_debug_message(temp_message);

    //int timestamp = interest_pkt.parameters.value[0];
    //printf("TIMESTAMP: %d\n", timestamp);
    int current_time = ndn_time_now_ms();
    //printf("LAST INTEREST: %d\n", last_interest);
    
    //this means that array[0] = anchor number 1
    //selector number
    prefix = get_prefix_component(interest_pkt.name, 1);
    prefix = trimwhitespace(prefix);
    //parameters is second
    int parameters = (atoi(prefix) - 1);
    printf("SELECTOR: %d\n", parameters);
    printf("STORED SELECTOR: %d\n", stored_selectors[parameters]);

    delay_struct_t args;
    args.interest = interest_pkt;
    args.struct_selector = parameters;

    //make sure to uncomment verify 
    // if(verify_interest(&interest_pkt) == false) {
    //     printf("Packet Wrong Format!");
    //     return NDN_UNSUPPORTED_FORMAT;
    // }
    //printf("Packet Verified!\n");

    //check ancmt, stored selectors, and timestamp(maybe)
    //timestamp + selector for new and old
    //TODO: fix time to correspond to last ancmt timestamp, fix timestamp in general
    //if((prefix == "ancmt") && stored_selectors[parameters] == false && (timestamp - last_interest) > 0) {

    //should be first slot in prefix
    prefix = get_prefix_component(interest_pkt.name, 0);
    prefix = trimwhitespace(prefix);

    if(strcmp(prefix, "ancmt") == 0) {
        char size_message[20] = "";
        char i_size[10] = "";
        sprintf(i_size, "%d", interest_size);
        strcat(size_message, "i1size:");
        strcat(size_message, i_size);
        strcat(size_message, " ; ");
        send_debug_message(size_message);
    }

    else if(strcmp(prefix, "l2interest") == 0) {
        char size_message[20] = "";
        char i_size[10] = "";
        sprintf(i_size, "%d", interest_size);
        strcat(size_message, "i2size:");
        strcat(size_message, i_size);
        strcat(size_message, " ; ");
        send_debug_message(size_message);
    }

    //---------------------------------------------------

    //stored selectors == false means selector is not in the bool array yet
    if(strcmp(prefix, "ancmt") == 0 && stored_selectors[parameters] == false) {
        printf("New Ancmt\n");
        stored_selectors[parameters] = true;
        if(delay_start[parameters] != true) {
            pthread_create(&layer1, NULL, &start_delay, (void *)&args);
            delay_start[parameters] = true;
        }
        interface_num[parameters]++;

        prefix = get_prefix_component(interest_pkt.name, 2);
        prefix = trimwhitespace(prefix);

    }

    else if(strcmp(prefix, "ancmt") == 0 && stored_selectors[parameters] == true) {
        printf("Old Ancmt\n");
        interface_num[parameters]++;
        if(did_flood[parameters] == true) {
            printf("Already flooded\n");
        }
        else if(interface_num[parameters] >= max_interfaces) {
            if(did_flood[parameters] == true) {
                printf("Already flooded\n");
            }
            else {
                flood(interest_pkt, get_prefix_component(interest_pkt.name, 1));
                printf("Maximum Interfaces Reached\n");
                did_flood[parameters] = true;
                prefix = get_prefix_component(interest_pkt.name, 1);
                // if(is_anchor == false) {
                if(atoi(prefix) != node_num) {
                    reply_ancmt(prefix);
                }
                //pthread_exit(NULL);
            }
        }
    }

    last_interest = current_time;
    printf("\nEND OF ON_INTEREST\n");

    return NDN_FWD_STRATEGY_SUPPRESS;
}

//if we reuse faces when sending and receiving packets, then msgqueue will also be scalable
ndn_udp_face_t *generate_udp_face(char* input_ip, char *port_1, char *port_2) {
    ndn_udp_face_t *face;

    in_port_t port1, port2;
    in_addr_t server_ip;
    char *sz_port1, *sz_port2, *sz_addr;
    uint32_t ul_port;
    struct hostent * host_addr;
    struct in_addr ** paddrs;

    sz_port1 = port_1;
    sz_addr = input_ip;
    sz_port2 = port_2;
    host_addr = gethostbyname(sz_addr);
    paddrs = (struct in_addr **)host_addr->h_addr_list;
    server_ip = paddrs[0]->s_addr;
    ul_port = strtoul(sz_port1, NULL, 10);
    port1 = htons((uint16_t) ul_port);
    ul_port = strtoul(sz_port2, NULL, 10);
    port2 = htons((uint16_t) ul_port);

    //return face;

    size_t udp_table_size = sizeof(udp_table.entries)/sizeof(udp_table.entries[0]);
    for(size_t i = 0; i < udp_table_size; i++) {    
        if(udp_table.entries[i].is_filled == false) {
            printf("Constructing new face [%d]: %s, %s, %s\n", i, input_ip, port_1, port_2);
            face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);
            udp_table.entries[i].face_entry = face;
            udp_table.entries[i].is_filled = true;
            return face;
        }
        else {
            char check_ip[40] = "";
            uint32_t input = udp_table.entries[i].face_entry->remote_addr.sin_addr.s_addr;
            inet_ntop(AF_INET, &input, check_ip, INET_ADDRSTRLEN);
            if(strcmp(input_ip, check_ip) == 0 && atoi(port_1) == htons(udp_table.entries[i].face_entry->local_addr.sin_port) && atoi(port_2) == htons(udp_table.entries[i].face_entry->remote_addr.sin_port)) {
                //printf("Found old face[%d]: %s, %d, %d\n", i, check_ip, htons(udp_table.entries[i].face_entry->local_addr.sin_port), htons(udp_table.entries[i].face_entry->remote_addr.sin_port));
                face = udp_table.entries[i].face_entry;
                return face;
            }
        }
    }

    return face;
}

void register_interest_prefix(char *input_prefix) {
    ndn_name_t name_prefix;
    char *ancmt_string = "";
    ancmt_string = input_prefix;
    ndn_name_from_string(&name_prefix, ancmt_string, strlen(ancmt_string));
    ndn_forwarder_register_name_prefix(&name_prefix, on_interest, NULL);
}

//NOTE: for recieving an incoming interest packet change the prefix string to the nodes that you want to recieve from
//also to send a interest packet, change the outgoing interest packet prefix
void populate_incoming_fib() {
    printf("\nIncoming FIB populated\nNOTE: all other nodes must be turned on and in the network, else SegFault \n");
    ndn_udp_face_t *face;

    //generate_udp_face here, then, use udp face for all other processes by only using udp face table
    //example neighbor(2):
    //face = generate(NODE2, "3000", "5000");
    //face = generate(NODE2, "5000", "3000");
    //face = generate(NODE2, "4000", "6000");
    //face = generate(NODE2, "6000", "4000");
    size_t nl_size = sizeof(neighbor_list)/sizeof(neighbor_list[0]);
    for(size_t i = 0; i < nl_size; i++) {
        if(neighbor_list[i] != 0) {
            char *ip_num = "";
            ip_num = search_ip_table(neighbor_list[i]);
            face = generate_udp_face(ip_num, "3000", "5000");
            face = generate_udp_face(ip_num, "5000", "3000");
            face = generate_udp_face(ip_num, "4000", "6000");
            face = generate_udp_face(ip_num, "6000", "4000");
        }
    } 
    // register_interest_prefix("/ancmt/1/1");
    // register_interest_prefix("/l2interest/1/2");
}

//check adding to array to store face and check if pointers are different
//create individual faces for each one and add them to an array to see if it changes
//look at udp face code to see if we can change it 

void insert_entry(anchor_pit_entry_t entry) {
    //TODO: use static variable to store last index so that we do not have to search through array every single time we want to insert into array
    //increment static variable by 1 every time we want to insert into array for next tie
    int entry_pos;
    size_t nap_size = sizeof(node_anchor_pit.slots)/sizeof(node_anchor_pit.slots[0]);
    for(size_t i = 0; i < nap_size; i++) {
        if(strcmp(node_anchor_pit.slots[i].prefix, "") == 0) {
            printf("Inserted Entry at POS: %d\n", i);
            entry_pos = i;
            node_anchor_pit.slots[i] = entry;
            return;
        }
    }
}

void fill_pit(const uint8_t* interest, uint32_t interest_size) {
    printf("\nFill Pit.\n");
    ndn_interest_t interest_pkt;
    anchor_pit_entry_t entry;
    char *insert_prefix = "";

    ndn_interest_from_block(&interest_pkt, interest, interest_size);

    //we only care about ndn name, and we can search ip table during fill pit to put ip string inside of pit entry
    insert_prefix = get_string_prefix(interest_pkt.name);
    printf("PIT PREFIX: %s\n", insert_prefix);
    ndn_name_print(&interest_pkt.name);
    printf("FILL PIT PACKET SIZE: %d\n", interest_size);

    char *cmp_string = "";
    cmp_string = get_prefix_component(interest_pkt.name, 0);
    int anchor_num;
    //node 1 = ancmt_num[0]
    anchor_num = atoi(get_prefix_component(interest_pkt.name, 1)) - 1;

    if(strcmp(cmp_string, "ancmt") == 0 && ancmt_num[anchor_num] < max_interfaces && did_flood[anchor_num] == false) {
        ancmt_num[anchor_num]++;
        printf("FILL PIT ANCMT NUM: %d\n", ancmt_num[anchor_num]);
        entry.name_struct = interest_pkt.name;
        entry.prefix = insert_prefix;

        insert_entry(entry);
        
    }
    else if(strcmp(cmp_string, "l2interest") == 0) {
        printf("FILL PIT L2INTEREST\n");
        entry.name_struct = interest_pkt.name;
        entry.prefix = insert_prefix;

        insert_entry(entry);
    }
    else {
        printf("Max Ancmt Fill Pit\n");
    }
}

// uint8_t *get_data_content(ndn_data_t input_packet, int start_index, int end_index) {
//     //end and start indexes are inclusive
//     //ex: start_index = 0, end_index = 4 ; means that we want content_value[0] -> content_value[4] and return array of size 5
//     printf("Getting Data Content: %d to %d\n", start_index, end_index);
//     int array_size = end_index - start_index + 1;
//     uint8_t return_array[array_size];
//     int j = 0;
//     for(int i = start_index; i <= end_index; i++) {
//         return_array[j] = input_packet.content_value[i];
//         j++;
//     }
//     return return_array;
// }

//is only called for anchors when assigning layer 1 data indexes
int insert_data_index(ndn_data_t input_data) {
    //intitialize all data values to 1024 and maybe call realloc(input_data.content_size)
    size_t cs_size = sizeof(cs_table.data_indexes)/sizeof(cs_table.data_indexes[0]);
    for(size_t i = 0; i < cs_size; i++) {
        if(strcmp(cs_table.data_indexes[i].data_value, input_data.content_value) == 0) {
            return -1;
        }
        if (cs_table.data_indexes[i].is_filled == false) {
            printf("ANCHOR DATA 1 INDEX: %d\n", i);
            memcpy(cs_table.data_indexes[i].data_value, input_data.content_value, input_data.content_size);
            cs_table.data_indexes[i].is_filled = true;
            return (int)i;
        }
    }
    return -2;
}

//only called if a data2 packet is received
int check_content_store(ndn_data_t *input_data, int outgoing_num) {
    //insert content store checking here
    //maybe use pointers for input data and then reuturn input_data

    //recv:  /l2data/anchor/sender (with) [bit_vector][index_num][content]
    size_t cs_size = sizeof(cs_table.entries)/sizeof(cs_table.entries[0]);
    for(size_t i = 0; i < cs_size; i++) {
        
        //if duplicate data
        if(strcmp(&input_data->content_value[7], &cs_table.entries[i].data_pkt.content_value[7]) == 0) {
            printf("DUPLICATE DATA FOUND IN CS\n");
            //update bit vector and send with new vector packet
            uint8_t temp_buffer[5] = {0};
            uint8_t vector_gen_buffer[1024] = {0};

            //link outgoing num to anchornum and index

            //ex: index 0 is anchor 1
            //put the data1 index from duplicate data2 packet into cstable.entries.data1_array
            int anchor_num_vector = atoi(get_prefix_component(input_data->name, 1));
            memcpy(&cs_table.entries[i].data1_array[anchor_num_vector-1].index[0], &input_data->content_value[5], 2);

            //bitwise OR the bit vector from duplicate with the bit vector inside current entry
            memcpy(&temp_buffer[0], &input_data->content_value[0], 5);
            for(int j = 0; j < 5; j++) {
                temp_buffer[j] = temp_buffer[j] | cs_table.entries[i].vector_num[j];
            }
            memcpy(&cs_table.entries[i].vector_num[0] ,&temp_buffer[0], 5);

            //set content of vector packet with new information [bit_vector][anchor(old)][index(old)][index(new)]
            memcpy(&vector_gen_buffer[0], &temp_buffer[0], 5);
            size_t index_size = sizeof(cs_table.entries[0].data1_array)/sizeof(cs_table.entries[0].data1_array[0]);
            for(size_t k = 0; k < index_size; k++) {
                if(cs_table.entries[i].data1_array[k].is_filled == true) {
                    // data_buffer[5] = (index_num >> 8) & 0xff;
                    // data_buffer[6] = index_num & 0xff;
                    vector_gen_buffer[5] = ((k+1) >> 8) & 0xff;
                    vector_gen_buffer[6] = (k+1) & 0xff;
                    memcpy(&vector_gen_buffer[7], &cs_table.entries[i].data1_array[k].index[0], 2);
                    break;
                }
            }
            memcpy(&vector_gen_buffer[9], &input_data->content_value[5], 2);

            ndn_data_set_content(input_data, vector_gen_buffer, 11);

            //updating data1 indexes array
            // size_t index_size = sizeof(cs_table.entries[0].data1_array)/sizeof(cs_table.entries[0].data1_array[0]);
            // for(int j = 0; j < index_size; j++) {
            //     if(cs_table.entries[i].data1_array[j].is_filled == false) {
            //         memcpy(cs_table.entries[i].data1_array[j].index, &input_data->content_value[5], 2);
            //         cs_table.entries[i].data1_array[j].is_filled = true;
            //     }
            // }

            //generate vector packet (modify input packet)
            return 0;
        }

        //if not duplicate data
        else {
            if(cs_table.entries[i].is_filled == false) {
                printf("CONTENT STORE DATA 2 INSERT INDEX: %d\n", i);

                //set data packet and the bit vector
                cs_table.entries[i].data_pkt = *input_data;
                cs_table.entries[i].is_filled = true;
                memcpy(&cs_table.entries[i].vector_num[0], &input_data->content_value[0], 5);

                //ex: index 0 is anchor 1
                //put the data1 index from data2 packet into cstable.entries.data1_array
                int anchor_num_data2 = atoi(get_prefix_component(input_data->name, 1));
                memcpy(&cs_table.entries[i].data1_array[anchor_num_data2-1].index[0], &input_data->content_value[5], 2);
                cs_table.entries[i].data1_array[anchor_num_data2-1].is_filled = true;

                // size_t index_size = sizeof(cs_table.entries[0].data1_array)/sizeof(cs_table.entries[0].data1_array[0]);
                // for(size_t j = 0; j < index_size; j++) {
                //     if(cs_table.entries[i].data1_array[j].is_filled == false) {
                //         memcpy(&cs_table.entries[i].data1_array[j].index[0], &input_data->content_value[5], 2);
                //         cs_table.entries[i].data1_array[j].is_filled = true;
                //     }
                // }

                return -1;
            }
        }
    }
    //only goes here if no duplicate and cs_table.entries is full

}

int vector_cs_store(ndn_data_t input_vector_packet) {
    printf("Vector CS updating\n");
    size_t cs_size = sizeof(cs_table.entries)/sizeof(cs_table.entries[0]);
    for(size_t i = 0; i < cs_size; i++) {
        //maybe instead of having 2 for loops, have a field inside entries that stores a set anchor(old) and a set index(old) so we dont have to search entire cs_table
        size_t index_size = sizeof(cs_table.entries[0].data1_array)/sizeof(cs_table.entries[0].data1_array[0]);
        for(size_t j = 0; j < index_size; j++) {
            //find correct entry and update bit vector and update index(new) and anchor(new) in cs_table.entries.data1_array
        }
    }
    return 0;
}

//TODO: clean out pit, cstable, and face table every time slot or periodically

//https://stackoverflow.com/questions/1163624/memcpy-with-startindex

//NOTE: IF PACKETS NOT SENDING TRY AGAIN AFTER CHECKING MSGQUEUE SIZE msg-queue.h
//might have to add udp faces to a table to solve this problem so we can reuse faces instead of creating new ones every time
void on_data(const uint8_t* rawdata, uint32_t data_size, void* userdata) {
    printf("On data\n");

    ndn_data_t data;
    ndn_encoder_t encoder;
    uint8_t buf[4096];
    char *ancmt_string;
    ndn_name_t name_prefix;
    ndn_udp_face_t *face;
    
    if (ndn_data_tlv_decode_digest_verify(&data, rawdata, data_size)) {
        printf("Decoding failed.\n");
    }

    char *prefix = "";
    prefix = get_string_prefix(data.name);
    printf("%s\n", prefix);

    printf("DATA CONTENT: [%s]\n", data.content_value);
    printf("Content Size Field: %d\n", data.content_size);

    for(int i = 0; i < data.content_size; i++) {
        printf(BYTE_TO_BINARY_PATTERN,BYTE_TO_BINARY(data.content_value[i]));
        printf(" ");
    }
    printf("\n");

    for(int i = 0; i < data.content_size; i++) {
        printf("%d",data.content_value[i]);
        printf(" ");
    }
    printf("\n");
    printf("Packet Size: %d\n", data_size);

    char temp_message[80] = "";
    strcat(temp_message, "On Data: ");
    strcat(temp_message, prefix);
    strcat(temp_message, " ; ");
    send_debug_message(temp_message);

    char *first_slot = "";
    first_slot = get_prefix_component(data.name, 0);

    char *second_slot_anchor = "";
    second_slot_anchor = get_prefix_component(data.name, 1);

    int third_slot = 0;
    
    if(strcmp(first_slot, "l1data") == 0) {

        char size_message[20] = "";
        char d_size[10] = "";
        sprintf(d_size, "%d", data_size);
        strcat(size_message, "d1size:");
        strcat(size_message, d_size);
        strcat(size_message, " ; ");
        send_debug_message(size_message);

        if(atoi(second_slot_anchor) == node_num) {
            printf("Anchor Layer 1 Data Received\n");

            char *in = "";
            in = timestamp();

            char pub_message[100] = "";
            strcat(pub_message, "2:");
            strcat(pub_message, in);
            strcat(pub_message, ">");
            strcat(pub_message, &data.content_value[0]);
            strcat(pub_message, "_");
            char tp_size[10] = "";
            sprintf(tp_size, "%d", data_size);
            strcat(pub_message, tp_size);
            strcat(pub_message, " ; ");
            send_debug_message(pub_message);
            
            //insert into anchor layer 1 data store with index (this is is only for anchors)
            int index_num = insert_data_index(data);
            if(index_num == -1) {
                //if -1, then cs data index is full or duplicate found if so, then do not flood
                printf("Data Index Error\n");
                return;
            }

            //each hex digit is 4 bits
            uint8_t data_buffer[1024] = {0};
            int second_num = atoi(second_slot_anchor);
            int insert_index = 4 - ((second_num-1) / 8);
            int insert_bit = (second_num - 1) % 8;

            //sets initial bit vector
            data_buffer[insert_index] = (int)(pow(2,insert_bit) + 1e-9);

            //sets the data 1 index inside data buffer
            data_buffer[5] = (index_num >> 8) & 0xff;
            data_buffer[6] = index_num & 0xff;

            //memcpy( &dst[dstIdx], &src[srcIdx], numElementsToCopy * sizeof( Element ) );
            memcpy(&data_buffer[7], &data.content_value[0], data.content_size);

            int l2_face_index;
            bool l2_interest_in = false;

            size_t nap_size = sizeof(node_anchor_pit.slots)/sizeof(node_anchor_pit.slots[0]);
            for(size_t i = 0; i < nap_size; i++) {
                char *check_string = "";
                check_string = get_prefix_component(node_anchor_pit.slots[i].name_struct, 0);
                char *check_ancmt_anchor = "";
                check_ancmt_anchor =  get_prefix_component(node_anchor_pit.slots[i].name_struct, 1);
                if(strcmp(check_string, "l2interest") == 0 && atoi(check_ancmt_anchor) == atoi(second_slot_anchor)) {
                    l2_face_index = i;

                    char* inputIP = "";
                    third_slot = atoi(get_prefix_component(node_anchor_pit.slots[i].name_struct, 2));
                    inputIP = search_ip_table(third_slot);
                    
                    generate_layer_2_data(inputIP, second_slot_anchor, data_buffer, data.content_size);
                    l2_interest_in = true;
                }
            }

            if(l2_interest_in == false) {
                printf("No layer 2 interest in Anchor\n");
            }
        }

        else {
            //no change for bit vector implementation
            printf("Node Layer 1 Data Received\n");
            int reply[10];
            int counter = 0;
            bool ancmt_in = false;

            size_t nap_size = sizeof(node_anchor_pit.slots)/sizeof(node_anchor_pit.slots[0]);
            for(size_t i = 0; i < nap_size; i++) {
                char *check_ancmt = "";
                check_ancmt = get_prefix_component(node_anchor_pit.slots[i].name_struct, 0);
                char *check_ancmt_anchor = "";
                check_ancmt_anchor =  get_prefix_component(node_anchor_pit.slots[i].name_struct, 1);
                if(strcmp(check_ancmt, "ancmt") == 0 && atoi(check_ancmt_anchor) == atoi(second_slot_anchor)) {
                    reply[counter] = atoi(get_prefix_component(node_anchor_pit.slots[i].name_struct, 2));
                    counter++;
                    ancmt_in = true;
                }
            }
            
            if(ancmt_in == true) {
                srand(time(0));
                int rand_num = rand() % counter;

                char *ip_string;
                ip_string = search_ip_table(reply[rand_num]);

                char change_num[20] = "";
                sprintf(change_num, "%d", node_num);
                char prefix_string[40] = "/l1data/";
                strcat(prefix_string, second_slot_anchor);
                strcat(prefix_string, "/");
                strcat(prefix_string, change_num);
                ndn_name_from_string(&name_prefix, prefix_string, strlen(prefix_string));
                data.name = name_prefix;

                encoder_init(&encoder, buf, 4096);
                ndn_data_tlv_encode_digest_sign(&encoder, &data);
                face = generate_udp_face(ip_string, "5000", "3000");
                ndn_face_send(&face->intf, encoder.output_value, encoder.offset);
                printf("Layer 1 Data Forwarded\n");

                send_debug_message("Layer 1 Data Forwarded ; ");
            }

            else {
                printf("No ancmt received\n");
            }
        }
    }

    //TODO: need to have 2 for loops for generation of bit vector to assign outgoing faces to index(old) for vector -> use array for all outgoing faces
    else if(strcmp(first_slot, "l2data") == 0) {
        printf("Layer 2 Data Recieved\n");
        //need to check cs if duplicate data has already been received before
        //need to add extra fields into cs: bit vector, data(content_value), data1 index array

        char size_message[20] = "";
        char d_size[10] = "";
        sprintf(d_size, "%d", data_size);
        strcat(size_message, "d2size:");
        strcat(size_message, d_size);
        strcat(size_message, " ; ");
        send_debug_message(size_message);

        int l2_face_index = 0;
        bool l2_interest_in = false;

        printf("DATA CONTENT (Layer 2 Data): [%s]\n", &data.content_value[7]);

        char *in = "";
        in = timestamp();

        char pub_message[100] = "";
        strcat(pub_message, "2:");
        strcat(pub_message, in);
        strcat(pub_message, ">");
        strcat(pub_message, &data.content_value[7]);
        strcat(pub_message, "_");
        char tp_size[10] = "";
        sprintf(tp_size, "%d", data_size);
        strcat(pub_message, tp_size);
        strcat(pub_message, " ; ");
        send_debug_message(pub_message);

        int cs_check = check_content_store(&data, 0);

        size_t nap_size = sizeof(node_anchor_pit.slots)/sizeof(node_anchor_pit.slots[0]);
        for(size_t i = 0; i < nap_size; i++) {
            char *check_string = "";
            check_string = get_prefix_component(node_anchor_pit.slots[i].name_struct, 0);
            char *check_anchor = "";
            check_anchor = get_prefix_component(node_anchor_pit.slots[i].name_struct, 1);
            if(strcmp(check_string, "l2interest") == 0 && atoi(check_anchor) == atoi(second_slot_anchor)) {
                l2_face_index = i;

                char *ip_string = "";
                third_slot = atoi(get_prefix_component(node_anchor_pit.slots[i].name_struct, 2));
                ip_string = search_ip_table(third_slot);

                char change_num[20] = "";
                sprintf(change_num, "%d", node_num);
                //char prefix_string[40] = "/l2data/";
                char prefix_string[40] = "";

                //using &data as input to directly change the packet without having to return a packet
                //include third slot here as the outgoing face node num
                //TODO: move this outside of the for loop and add link index old with outgoing faces
                //int cs_check = check_content_store(&data, third_slot);

                //data is duplicated (OR the bit vector)
                if(cs_check ==  0) {
                    strcat(prefix_string, "/vector/");
                    //vector: bit_vector(5)->anchor_num_old(2)->data_index_old(2)->data_index_new(2) and then associate data_index_new with the second slot anchor prefix to udpate cs index array

                }
                //data is not duplicated
                else {
                    strcat(prefix_string, "/l2data/");
                    //data content should be forwarded the same if data not in cs first
                    //good after we strcat /l2data/ data_content forwarded should be the same
                }

                strcat(prefix_string, second_slot_anchor);
                strcat(prefix_string, "/");
                strcat(prefix_string, change_num);
                ndn_name_from_string(&name_prefix, prefix_string, strlen(prefix_string));
                data.name = name_prefix;

                encoder_init(&encoder, buf, 4096);
                ndn_data_tlv_encode_digest_sign(&encoder, &data);
                face = generate_udp_face(ip_string, "6000", "4000");
                ndn_face_send(&face->intf, encoder.output_value, encoder.offset);
                printf("Layer 2 Data Forwarded\n");

                send_debug_message("Layer 2 Data Forwarded ; ");
                l2_interest_in = true;
            }
        }

        if(l2_interest_in == false) {
            printf("No layer 2 interest\n");
        }
    }

    else if(strcmp(first_slot, "vector") == 0) {
        printf("Vector Packet Recieved\n");

        //update content store from bit vector and forward updated bit vector, bit vector recevieced should be bit vector sent 9)
        //vector: /bit_vector(5)/anchor_num_old(2)/data_index_old(2)/data_index_new(2)/ and then associate data_index_new with the second slot anchor prefix to udpate cs index array

        char size_message[20] = "";
        char d_size[10] = "";
        sprintf(d_size, "%d", data_size);
        strcat(size_message, "vecsize:");
        strcat(size_message, d_size);
        strcat(size_message, " ; ");
        send_debug_message(size_message);

        int l2_face_index = 0;
        bool l2_interest_in = false;

        int recv_vector = vector_cs_store(data);

        size_t nap_size = sizeof(node_anchor_pit.slots)/sizeof(node_anchor_pit.slots[0]);
        for(size_t i = 0; i < nap_size; i++) {
            char *check_string = "";
            check_string = get_prefix_component(node_anchor_pit.slots[i].name_struct, 0);
            char *check_anchor = "";
            check_anchor = get_prefix_component(node_anchor_pit.slots[i].name_struct, 1);
            if(strcmp(check_string, "l2interest") == 0 && atoi(check_anchor) == atoi(second_slot_anchor)) {
                l2_face_index = i;

                char *ip_string = "";
                third_slot = atoi(get_prefix_component(node_anchor_pit.slots[i].name_struct, 2));
                ip_string = search_ip_table(third_slot);

                char change_num[20] = "";
                sprintf(change_num, "%d", node_num);
                char prefix_string[40] = "/vector/";
                strcat(prefix_string, second_slot_anchor);
                strcat(prefix_string, "/");
                strcat(prefix_string, change_num);
                ndn_name_from_string(&name_prefix, prefix_string, strlen(prefix_string));
                data.name = name_prefix;

                encoder_init(&encoder, buf, 4096);
                ndn_data_tlv_encode_digest_sign(&encoder, &data);
                face = generate_udp_face(ip_string, "6000", "4000");
                ndn_face_send(&face->intf, encoder.output_value, encoder.offset);
                printf("Vector Forwarded\n");

                send_debug_message("Vector Forwarded ; ");
                l2_interest_in = true;
            }
        }

        if(l2_interest_in == false) {
            printf("No layer 2 interest\n");
        }
    }
}

//interest is saved in pit until put-Data is called

/*
bool verify_data(ndn_data_t *data_pkt, const uint8_t* rawdata, uint32_t data_size) {

}

void select_anchor() {

}
*/

//write to mongodb so that we can generate web server to view pit

void *forwarding_process(void *var) {
    running = true;
    while (running) {
        ndn_forwarder_process();
        usleep(10000);
    }
}

void *command_process(void *var) {
    int select = 1;
    while(select != 0) {
        printf("0: Exit\n2: Generate Layer 1 Data\n3: Generate UDP Face(Check Face Valid)\n4: Flood To Neighbors\n5: Connect to debug server\n6: Latency and Throughput Test\n");
        scanf("%d", &select);
        printf("SELECT: %d\n", select);
        switch (select) {
            case 0: {
                printf("Exiting\n");
                break;
            }
                
            case 2: {
                //malloc 40 = 40 bytes
                char *input_string = malloc(40);
                printf("Generate Data -> Please input data string:\n");
                scanf("%s", input_string);
                printf("Generate Data Text Input: %s\n", input_string);
                send_debug_message("Clear Graph");
                clock_t debug_timer = clock();
                while (clock() < (debug_timer + 1000000)) {
                }
                generate_data(input_string);
                break;
            }

            case 3: {
                printf("Check Face Valid\n");
                //ndn_udp_face_t *face;
                //face = generate_udp_face("0.0.0.0", "7000", "8000");
                // if(udp_table.entries[30].face_entry == NULL) {
                //     printf("Entry is null\n");
                // }
                //false = 0
                size_t udp_table_size = sizeof(udp_table.entries)/sizeof(udp_table.entries[0]);
                for(size_t i = 0; i < udp_table_size; i++) {
                    printf("Is filled [%d]: %d\t", i, udp_table.entries[i].is_filled);
                    if(udp_table.entries[i].face_entry == NULL) {
                        printf("UDP face [%d]: %s", i, "NULL");
                    }
                    else {
                        char check_ip[40] = "";
                        uint32_t input = udp_table.entries[i].face_entry->remote_addr.sin_addr.s_addr;
                        inet_ntop(AF_INET, &input, check_ip, INET_ADDRSTRLEN);
                        printf("UDP face [%d]: %s, %d, %d", i, check_ip, 
                        htons(udp_table.entries[i].face_entry->local_addr.sin_port), 
                        htons(udp_table.entries[i].face_entry->remote_addr.sin_port));
                    }
                    printf("\n");
                    // udp_table.entries[i].face_entry = NULL;
                }
                ndn_udp_face_t *test = generate_udp_face(NODE5, "3000", "5000");
                break;
            }

            case 4: {
                printf("Anchor init flooding\n");
                is_anchor = true;
                ndn_interest_t interest;
                char *temp_char;
                temp_char = malloc(10);
                temp_char[0] = 0;
                sprintf(temp_char, "%d", node_num);
                flood(interest, temp_char);
                break;
            }

            case 5: {
                printf("Connecting to Debug Server\n");

                //socket connection
                if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                {
                    printf("\n Socket creation error \n");
                }

                // int flags = 1;
                // if (setsockopt(sock, SOL_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags))) { 
                //     printf("\nERROR: setsocketopt(), TCP_NODELAY\n");
                //     exit(0); 
                // }
            
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(PORT);

                if(inet_pton(AF_INET, DEBUG, &serv_addr.sin_addr)<=0) 
                {
                    printf("\nInvalid address/ Address not supported \n");
                }
            
                if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                {
                    printf("\nConnection Failed \n");
                }

                debug_connected = true;
                break;
            }

            case 11: {
                printf("Connecting to Debug Server\n");

                //socket connection
                if ((sock_graph = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                {
                    printf("\n Socket creation error \n");
                }

                // int flags = 1;
                // if (setsockopt(sock, SOL_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags))) { 
                //     printf("\nERROR: setsocketopt(), TCP_NODELAY\n");
                //     exit(0); 
                // }
            
                serv_addr_graph.sin_family = AF_INET;
                serv_addr_graph.sin_port = htons(8887);

                if(inet_pton(AF_INET, DEBUG, &serv_addr_graph.sin_addr)<=0) 
                {
                    printf("\nInvalid address/ Address not supported \n");
                }
            
                if (connect(sock_graph, (struct sockaddr *)&serv_addr_graph, sizeof(serv_addr_graph)) < 0)
                {
                    printf("\nConnection Failed \n");
                }

                debug_connected_graph = true;
                break;
            }

            case 6: {
                // char *input_string = malloc(40);
                // printf("Generate Data -> Please input data string:\n");
                // scanf("%s", input_string);
                // printf("Generate Data Text Input: %s\n", input_string);
                // send_debug_message("Clear Graph");
                // clock_t debug_timer = clock();
                // while (clock() < (debug_timer + 5000000)) {
                // }
                // generate_data(input_string);
                // break;

                // char pub_message[100] = "";
                // strcat(pub_message, "Layer 2 Data Sent -> ");
                // // strcat(pub_message, data_string);
                // // strcat(pub_message, " -> ");
                // strcat(pub_message, in);
                // strcat(pub_message, " ; ");
                // send_debug_message(pub_message);

                printf("Latency and Throughput Test\n");

                for (int i = 0; i < 100; i++) {
                    clock_t latency_timer = clock();

                    char test_message[300] = "";
                    strcat(test_message, 
                    "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000test");
                    char test_num[10] = ""; 

                    while (clock() < (latency_timer + 1000000)) {
                    }

                    sprintf(test_num, "%d", i);
                    strcat(test_message, test_num);
                    generate_data(test_message);
                }
                break;
            }

            case 7: {
                printf("Find ad-hoc neighbors\n");  
                break;
            }

            case 8: {
                printf("Populate FIB\n");
                break;
            }

            case 9: {
                printf("Anchor Data Indexes Print\n");
                for(int i = 0; i < 20; i++) {
                    printf("%d: %s\n", i, cs_table.data_indexes[i].data_value);
                }
                break;
            }

            default: {
                printf("Invalid Input\n");
                break;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    printf("Main Loop\n");
    printf("Maximum Interfaces: %d\n", max_interfaces);

    pthread_t forwarding_process_thread;
    pthread_t command_process_thread;

    //TODO: make this a function later
    // char temp_message[80];
    // char temp_num[10];
    // strcat(temp_message, "Node Start: ");
    // sprintf(temp_num, "%d", node_num);
    // strcat(temp_message, temp_num);
    // send_debug_message(temp_message);

    //init pit
    size_t nap_size = sizeof(node_anchor_pit.slots)/sizeof(node_anchor_pit.slots[0]);
    for(size_t i = 0; i < nap_size; i++) {
        node_anchor_pit.slots[i].prefix = "";
    }
    size_t cs_size = sizeof(cs_table.entries)/sizeof(cs_table.entries[0]);
    for(size_t i = 0; i < cs_size; i++) {
        cs_table.entries[i].is_filled = false;
    }
    size_t udp_table_size = sizeof(udp_table.entries)/sizeof(udp_table.entries[0]);
    for(size_t i = 0; i < udp_table_size; i++) {
        udp_table.entries[i].is_filled = false;
        udp_table.entries[i].face_entry = NULL;
    }
    
    //replace this later with node discovery
    add_ip_table("1",NODE1);
    add_ip_table("2",NODE2);
    add_ip_table("3",NODE3);
    add_ip_table("4",NODE4);
    add_ip_table("5",NODE5);
    add_ip_table("6",NODE6);
    add_ip_table("7",NODE7);
    add_ip_table("8",NODE8);
    add_ip_table("9",NODE9);
    add_ip_table("10",NODE10);
    add_ip_table("11",NODE11);
    add_ip_table("12",NODE12);
    add_ip_table("13",NODE13);
    add_ip_table("14",NODE14);

    //start ndn_lite and init all data structures in forwarder
    ndn_lite_startup();

    //This is for adding 2 way neighbors in network
    //DEMO: CHANGE
    node_num = 7;
    add_neighbor(2);
    add_neighbor(4);
    add_neighbor(5);
    add_neighbor(9);
    add_neighbor(11);

    last_interest = ndn_time_now_ms();
    
    //FACE NEEDS TO BE INITIATED WITH CORRECT PARAMETERS BEFORE SENDING OR RECEIVING ANCMT
    //registers ancmt prefix with the forwarder so when ndn_forwarder_process is called, it will call the function on_interest
    populate_incoming_fib();
    callback_insert(on_interest, on_data, fill_pit);

    //signature init here

    //when production wants to send data and recieve packets, do thread for while loop and thread for sending data when producer wants to
    pthread_create(&forwarding_process_thread, NULL, forwarding_process, NULL);
    pthread_create(&command_process_thread, NULL, command_process, NULL);
    pthread_join(forwarding_process_thread, NULL);
    pthread_join(command_process_thread, NULL);

    return 0;
}