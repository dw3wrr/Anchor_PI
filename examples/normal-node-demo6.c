#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <setjmp.h>
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
#define NODE1 "155.246.44.89"
#define NODE2 "155.246.215.95"
#define NODE3 "155.246.202.140"
#define NODE4 "155.246.216.124"
#define NODE5 "155.246.203.173"
#define NODE6 "155.246.216.114"
#define NODE7 "155.246.202.144"
#define NODE8 "155.246.212.94"
#define NODE9 "155.246.213.124"
#define NODE10 "155.246.210.80"
#define DEBUG "155.246.182.138"

//in the build directory go to make files and normal node -change the link.txt
//CMAKE again
//then make
//link.txt
///usr/bin/cc  -std=c11 -Werror -Wno-format -Wno-int-to-void-pointer-cast -Wno-int-to-pointer-cast -O3   CMakeFiles/normal-node.dir/examples/normal-node.c.o  -pthread -o examples/normal-node  libndn-lite.a

ndn_udp_face_t *generate_udp_face(char* input_ip, char *port_1, char *port_2);

typedef struct anchor_pit_entry {
    ndn_name_t name_struct;
    char *prefix;
    ndn_face_intf_t *face;
    ndn_udp_face_t *udp_face;
    bool rand_flag;
} anchor_pit_entry_t;

//for linking prefixes to a specific face
typedef struct anchor_pit {
    //change size to be more dynamic when iterating through array
    int mem;
    anchor_pit_entry_t slots[20];
} anchor_pit_t;

typedef struct ip_table_entry {
    char *ip_address;
    char *node_num;
} ip_table_entry_t;

typedef struct ip_table {
    int last_entry;
    ip_table_entry_t entries[10];
} ip_table_t;

typedef struct udp_face_table_entry {
    ndn_udp_face_t *face_entry;
    bool is_filled;
} udp_face_table_entry_t;

typedef struct udp_face_table {
    int size;
    udp_face_table_entry_t entries[20];
} udp_face_table_t;

typedef struct bit_vector {
    int vector_num;
} bit_vector_t;

typedef struct content_store_entry {
    ndn_data_t data_pkt;
    bool is_filled;
} content_store_entry_t;

typedef struct content_store {
    int size;
    content_store_entry_t entries[20];
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

//To set whether this node is anchor
bool is_anchor = false;

//To set whether an announcement(interest) has been sent
bool ancmt_sent = false;

//Time Slice
int time_slice = 3;

//Selector integers
//selector will be set from hash function of previous block
//uint8_t selector[10] = {1,2,3,4,5,6,7,8,9,10}; //change from 0 to 9
//uint8_t *selector_ptr = selector;

bool stored_selectors[10];
bool delay_start[10];
//set array for multiple anchors for anchor/selector 1 - 10
int interface_num[10];
//for telling if a node has flooded for the specific anchor
bool did_flood[10];

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

//ndn_udp_face_t *face1, *face2, *face3, *face4, *face5, *face6, *face7, *face8, *face9, *face10, *data_face;

int ancmt_num = 0;

//node_num future use for the third slot in prefix
//DEMO: CHANGE
int node_num = 6;

int send_debug_message(char *input) {
    char *debug_message;
    //char buffer[1024] = {0};
    //int valread;
    debug_message = input;
    send(sock , debug_message, strlen(debug_message) , 0 );

    //printf("Hello message sent\n");
    //valread = read( sock , buffer, 1024);
    //printf("%s\n",buffer);
    return 0;
}

void add_ip_table(char *input_num, char *input_ip) {
    int num;
    num = atoi(input_num);
    ip_list.entries[num-1].node_num = input_num;
    ip_list.entries[num-1].ip_address = input_ip;
}

char *search_ip_table(int input_num) {
    printf("Search IP Table\n");
    char *return_var = "";
    printf("SEARCH INDEX: %d\n", input_num);
    return_var = ip_list.entries[input_num-1].ip_address;
    printf("RETURN IP: %s\n", return_var);
    return return_var;
}

//inet ntoa
char *get_ip_address_string(ndn_udp_face_t *input_face) {
    char *output = "";
    struct in_addr input;
    input = input_face->remote_addr.sin_addr;
    output = inet_ntoa(input);
    return output;
}

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

char *get_prefix_component(ndn_name_t input_name, int num_input) {
    printf("Get Prefix Component %d\n",num_input);
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
//TODO: also fix the fact that normal nodes flood

//may have to use interest as a pointer
void flood(ndn_interest_t interest_pkt) {
    printf("\nFlooding\n");
    ndn_interest_t interest;
    ndn_name_t prefix_name;
    ndn_udp_face_t *face;

    char change_num[20] = "";
    sprintf(change_num, "%d", node_num);
    char ancmt_string[20] = "/ancmt/1/";
    strcat(ancmt_string, change_num);
    ndn_name_from_string(&prefix_name, ancmt_string, strlen(ancmt_string));

    //DEMO: CHANGE
    face = generate_udp_face(NODE7, "3000", "5000");
    ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);

    face = generate_udp_face(NODE9, "3000", "5000");
    ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);

    ndn_interest_from_name(&interest, &prefix_name);
    ndn_forwarder_express_interest_struct(&interest, NULL, NULL, NULL);

    printf("Flooded Interest!\n");
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
//     //FIXED TODO: Segmentation Fault Here
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
//     //FIXED TODO: segmentation fault here
//     ndn_signed_interest_ecdsa_sign(&ancmt, self_identity_ptr, ecc_secp256r1_prv_key);
//     // encoder_init(&encoder, interest_buf, 4096);
//     // //FIXED TODO: Segmentation Fault Here
//     // ndn_interest_tlv_encode(&encoder, &ancmt);

//     //uncomment here to test flood
//     flood(ancmt);
//     ancmt_sent = true;
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
void reply_ancmt() {
    //send_debug_message("Announcent Reply Sent");
    printf("\nReply Ancmt...\n");
    int reply[10];
    int counter = 0;

    for(int i = 0; i < node_anchor_pit.mem; i++) {
        char *check_ancmt = "";
        check_ancmt = get_prefix_component(node_anchor_pit.slots[i].name_struct, 0);
        if(strcmp(check_ancmt, "ancmt") == 0){
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
    char ancmt_string[20] = "/l2interest/1/";
    strcat(ancmt_string, change_num);
    ndn_name_from_string(&prefix_name, ancmt_string, strlen(ancmt_string));

    face = generate_udp_face(ip_string, "4000", "6000");
    ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);

    ndn_interest_from_name(&interest, &prefix_name);
    ndn_forwarder_express_interest_struct(&interest, NULL, NULL, NULL);
}

//input is name
void generate_layer_2_data(char *input_ip) {
    printf("\nGenerate Layer 2 Data\n");
    ndn_data_t data;
    ndn_name_t prefix_name;
    ndn_udp_face_t *face;
    ndn_encoder_t encoder;
    char *str = "This is a Layer 2 Data Packet";
    uint8_t buf[4096];

    //prefix string can be anything here because data_recieve bypasses prefix check in fwd_data_pipeline
    char change_num[20] = "";
    sprintf(change_num, "%d", node_num);
    char prefix_string[40] = "/l2data/1/";
    strcat(prefix_string, change_num);
    ndn_name_from_string(&prefix_name, prefix_string, strlen(prefix_string));

    data.name = prefix_name;
    ndn_data_set_content(&data, (uint8_t*)str, strlen(str) + 1);
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
void generate_data() {
    printf("\nGenerate Layer 1 Data\n");
    ndn_data_t data;
    ndn_name_t prefix_name;
    ndn_udp_face_t *face;
    ndn_encoder_t encoder;
    char *str = "This is a Layer 1 Data Packet";
    uint8_t buf[4096];

    //prefix string can be anything here because data_recieve bypasses prefix check in fwd_data_pipeline
    char change_num[40] = "";
    sprintf(change_num, "%d", node_num);
    char prefix_string[20] = "/l1data/1/";
    strcat(prefix_string, change_num);
    ndn_name_from_string(&prefix_name, prefix_string, strlen(prefix_string));

    int reply[10];
    int counter = 0;

    for(int i = 0; i < node_anchor_pit.mem; i++) {
        char *check_ancmt = "";
        check_ancmt = get_prefix_component(node_anchor_pit.slots[i].name_struct, 0);
        if(strcmp(check_ancmt, "ancmt") == 0){
            reply[counter] = atoi(get_prefix_component(node_anchor_pit.slots[i].name_struct, 2));
            counter++;
        }
    }

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

    send_debug_message("Layer 1 Data Sent ; ");
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
        flood(args->interest);
        did_flood[args->struct_selector] = true;
        if(is_anchor == false) {
            reply_ancmt();
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

// void periodic_publish(int times) {
//     int num_pub = 1;
//     while(num_pub <= times) {
//         clock_t timer = clock();
//         while (clock() < (timer + 6000000)) {
//         }
//         //printf("Publish Times: %d", num_pub);
//         generate_data();
//         num_pub++;
//     }
// }

//is this threaded, if so, then
//non zero chance of flooding twice due to multithreading
int on_interest(const uint8_t* interest, uint32_t interest_size, void* userdata) {
    printf("\nNormal-Node On Interest\n");
    pthread_t layer1;
    //pthread_t per_pub;
    ndn_interest_t interest_pkt;
    ndn_interest_from_block(&interest_pkt, interest, interest_size);

    char *prefix = "";
    prefix = get_string_prefix(interest_pkt.name);
    printf("PREFIX: %s\n", prefix);

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
    //TODO: fix time to coorespond to last ancmt timestamp, fix timestamp in general
    //if((prefix == "ancmt") && stored_selectors[parameters] == false && (timestamp - last_interest) > 0) {

    //should be first slot in prefix
    prefix = get_prefix_component(interest_pkt.name, 0);
    prefix = trimwhitespace(prefix);

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
                flood(interest_pkt);
                printf("Maximum Interfaces Reached\n");
                did_flood[parameters] = true;
                if(is_anchor == false) {
                    reply_ancmt();
                }
                //DEMO: CHANGE
                //generate_data();
                //pthread_exit(NULL);
            }
        }
    }

    //if l2interest do nothing, fill pi is enough
    else if(strcmp(prefix, "l2interest") == 0) {

    }

    last_interest = current_time;
    printf("\nEND OF ON_INTEREST\n");

    return NDN_FWD_STRATEGY_SUPPRESS;
}

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
    face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);

    return face;

    /*
    ndn_udp_face_t *face;

    in_port_t port1, port2;
    in_addr_t server_ip;
    char *sz_port1, *sz_port2, *sz_addr;
    uint32_t ul_port;
    struct hostent * host_addr;
    struct in_addr ** paddrs;

    char *check_ip, *check_port_1, *check_port_2;
    check_ip = malloc(40);
    check_port_1[0] = 0;
    check_port_1 = malloc(40);
    check_port_1[0] = 0;
    check_port_2 = malloc(40);
    check_port_2[0] = 0;

    struct in_addr_t input;
    int last_face = 0;
    bool found = false;

    printf("1\n");
    for(int i = 0; i < 5; i++) {
        //maybe error because of nothing in the table
        input = udp_table.faces[i]->remote_addr.sin_addr.s_addr;
        printf("1\n");
        inet_ntop(AF_INET, input, check_ip, sizeof(check_ip));
        printf("1\n");
        printf("IP: %s, ", check_ip);
        sprintf(check_port_1, "%d", htons(udp_table.faces[i]->local_addr.sin_port));
        printf("PORT1: %s, ", check_port_1);
        sprintf(check_port_2, "%d", htons(udp_table.faces[i]->remote_addr.sin_port));
        printf("PORT2: %s\n", check_port_2);
        printf("CHECK IP: %s, CHECK PORT1: %s, CHECK PORT2: %s\n", input_ip, port_1, port_2);
        if(strcmp(input_ip, check_ip) == 0 && strcmp(port_1, check_port_1) == 0 && strcmp(port_2, check_port_2) == 0) {
            printf("Exiting\n");
            found = true;
            face = udp_table.faces[i];
            break;
        }
    }

    if(found == true) {
        printf("Face already in udp_face_table.\n");
    }

    else {
        printf("Face not in udp_face_table, creating new face\n");
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
        face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);
        printf("Added face to table at: %d\n", udp_table.last_udp);
        udp_table.faces[udp_table.last_udp] = face;
        udp_table.last_udp++;
    }

    return face;
    */
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

    //remove youre own incoming interface
    //change NODE(NUM) and face(num)
    //only need to add face for layer 1 incoming
    //DEMO: CHANGE
    face = generate_udp_face(NODE3, "5000", "3000");
    face = generate_udp_face(NODE4, "5000", "3000");
    face = generate_udp_face(NODE7, "6000", "4000");
    face = generate_udp_face(NODE9, "6000", "4000");
    register_interest_prefix("/ancmt/1/3");
    register_interest_prefix("/ancmt/1/4");
    register_interest_prefix("/l2interest/1/7");
    register_interest_prefix("/l2interest/1/9");
}

//check adding to array to store face and check if pointers are different
//create individual faces for each one and add them to an array to see if it changes
//look at udp face code to see if we can change it 

void insert_entry(anchor_pit_entry_t entry) {
    int entry_pos;
    for(int i = 0; i < node_anchor_pit.mem; i++) {
        if(strcmp(node_anchor_pit.slots[i].prefix, "") == 0) {
            printf("Inserted Entry at POS: %d\n", i);
            entry_pos = i;
            node_anchor_pit.slots[i] = entry;
            return;
        }
    }
}

void fill_pit(const uint8_t* interest, uint32_t interest_size, ndn_face_intf_t *face) {
    printf("\nFill Pit.\n");
    ndn_interest_t interest_pkt;
    anchor_pit_entry_t entry;
    char *insert_prefix = "";
    ndn_face_intf_t *input_face = face;

    ndn_interest_from_block(&interest_pkt, interest, interest_size);

    //we only care about ndn name, and we can search ip table during fill pit to put ip string inside of pit entry
    insert_prefix = get_string_prefix(interest_pkt.name);
    printf("PIT PREFIX: %s\n", insert_prefix);
    //printf("FILL FACE: %p\n", input_face);
    ndn_name_print(&interest_pkt.name);

    char *cmp_string = "";
    cmp_string = get_prefix_component(interest_pkt.name, 0);

    if(strcmp(cmp_string, "ancmt") == 0 && ancmt_num < max_interfaces) {
        ancmt_num++;
        printf("FILL PIT ANCMT NUM: %d\n", ancmt_num);

        entry.face = input_face;
        entry.name_struct = interest_pkt.name;
        entry.prefix = insert_prefix;

        insert_entry(entry);
        
    }
    else if(strcmp(cmp_string, "l2interest") == 0) {
        printf("FILL PIT L2INTEREST\n");
        entry.face = input_face;
        entry.name_struct = interest_pkt.name;
        entry.prefix = insert_prefix;

        insert_entry(entry);
    }
    else {
        printf("Max Ancmt Fill Pit\n");
    }
}

void insert_content_store(ndn_data_t input_data) {
    for(int i = 0; i < cs_table.size; i++) {
        if(cs_table.entries[i].is_filled == false) {
            printf("CONTENT STORE INSERT INDEX: %d\n", i);
            cs_table.entries[i].data_pkt = input_data;
            cs_table.entries[i].is_filled = true;
            break;
        }
    }
}

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
    printf("DATA CONTENT: %s\n", data.content_value);

    // prefix = get_prefix_component(data.name, 2);
    // prefix = trimwhitespace(prefix);
    // prefix = get_string_prefix(data.name);
    char temp_message[80] = "";
    strcat(temp_message, "On Data: ");
    strcat(temp_message, prefix);
    strcat(temp_message, " ; ");
    send_debug_message(temp_message);

    char *first_slot = "";
    first_slot = get_prefix_component(data.name, 0);
    int third_slot;
    
    if(strcmp(first_slot, "l1data") == 0) {
        if(is_anchor) {
            printf("Anchor Layer 1 Data Received\n");
            int l2_face_index;
            bool l2_interest_in = false;

            for(int i = 0; i < node_anchor_pit.mem; i++) {
                char *check_string = "";
                check_string = get_prefix_component(node_anchor_pit.slots[i].name_struct, 0);
                if(strcmp(check_string, "l2interest") == 0) {
                    l2_face_index = i;

                    third_slot = atoi(get_prefix_component(node_anchor_pit.slots[i].name_struct, 2));
                    char* inputIP = "";
                    inputIP = search_ip_table(third_slot);

                    // clock_t timer = clock();
                    // printf("Delay Time: %d seconds\n", 2);
                    // while (clock() < (timer + 2000000)) {
                    // }
                    
                    generate_layer_2_data(inputIP);
                    l2_interest_in = true;
                }
            }

            if(l2_interest_in == false) {
                printf("No layer 2 interest in Anchor\n");
            }
        }

        else {
            printf("Node Layer 1 Data Received\n");
            int reply[10];
            int counter = 0;
            bool ancmt_in = false;

            for(int i = 0; i < node_anchor_pit.mem; i++) {
                char *check_ancmt = "";
                check_ancmt = get_prefix_component(node_anchor_pit.slots[i].name_struct, 0);
                if(strcmp(check_ancmt, "ancmt") == 0) {
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

                // clock_t timer = clock();
                // printf("Delay Time: %d seconds\n", 1);
                // while (clock() < (timer + 1000000)) {
                // }

                char change_num[20] = "";
                sprintf(change_num, "%d", node_num);
                char prefix_string[40] = "/l1data/1/";
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

    else if(strcmp(first_slot, "l2data") == 0) {
        printf("Layer 2 Data Recieved\n");
        int l2_face_index = 0;
        bool l2_interest_in = false;

        insert_content_store(data);

        for(int i = 0; i < node_anchor_pit.mem; i++) {
            char *check_string = "";
            check_string = get_prefix_component(node_anchor_pit.slots[i].name_struct, 0);
            if(strcmp(check_string, "l2interest") == 0) {
                l2_face_index = i;

                third_slot = atoi(get_prefix_component(node_anchor_pit.slots[i].name_struct, 2));
                char *ip_string = "";
                ip_string = search_ip_table(third_slot);

                // clock_t timer = clock();
                // printf("Delay Time: %d seconds\n", 1);
                // while (clock() < (timer + 1000000)) {
                // }

                char change_num[20] = "";
                sprintf(change_num, "%d", node_num);
                char prefix_string[40] = "/l2data/1/";
                printf("Here\n");
                strcat(prefix_string, change_num);
                printf("Here\n");
                ndn_name_from_string(&name_prefix, prefix_string, strlen(prefix_string));
                data.name = name_prefix;

                printf("Here\n");
                encoder_init(&encoder, buf, 4096);
                ndn_data_tlv_encode_digest_sign(&encoder, &data);
                printf("Here\n");
                face = generate_udp_face(ip_string, "6000", "4000");
                printf("Here\n");
                ndn_face_send(&face->intf, encoder.output_value, encoder.offset);
                printf("Here\n");
                printf("Layer 2 Data Forwarded\n");

                send_debug_message("Layer 2 Data Forwarded ; ");
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

bool check_CS(ndn_data_t *data_pkt) {

}

void select_anchor() {

}
*/

//write to mongodb so that we can generate web server to view pit

void *forwarding_process(void *var) {
    running = true;
    while (running) {
        if(is_anchor && !ancmt_sent) {
            //printf("send ancmt called\n");
            ndn_interest_t interest;
            flood(interest);
            ancmt_sent = true;
        }
        ndn_forwarder_process();
        usleep(10000);
    }
}

void *command_process(void *var) {
    int select = 1;
    while(select != 0) {
        printf("0: Exit\n2: Generate Layer 1 Data\n3: Generate UDP Face\n");
        scanf("%d", &select);
        printf("SELECT: %d\n", select);
        switch (select) {
            case 0:
                printf("Exiting\n");
                break;

            case 2:
                printf("Generate Data\n");
                send_debug_message("Clear Graph");
                clock_t timer = clock();
                while (clock() < (timer + 5000000)) {
                }
                generate_data();
                break;

            case 3:
                printf("Generating Face\n");
                ndn_udp_face_t *face;
                face = generate_udp_face("0.0.0.0", "7000", "8000");
                break;

            default:
                printf("Invalid Input\n");
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    printf("Main Loop\n");
    printf("Maximum Interfaces: %d\n", max_interfaces);

    pthread_t forwarding_process_thread;
    pthread_t command_process_thread;

    //socket connection
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
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
        return -1;
    }
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    //TODO: make this a function later
    // char temp_message[80];
    // char temp_num[10];
    // strcat(temp_message, "Node Start: ");
    // sprintf(temp_num, "%d", node_num);
    // strcat(temp_message, temp_num);
    // send_debug_message(temp_message);

    //init pit
    node_anchor_pit.mem = 20;
    for(int i = 0; i < node_anchor_pit.mem; i++) {
        node_anchor_pit.slots[i].prefix = "";
        node_anchor_pit.slots[i].rand_flag = false;
    }
    cs_table.size = 20;
    for(int i = 0; i < cs_table.size; i++) {
        cs_table.entries[i].is_filled = false;
    }
    udp_table.size = 20;
    for(int i = 0; i < udp_table.size; i++) {
        udp_table.entries[i].is_filled = false;
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

    ndn_lite_startup();

    //initializing face_table
    /*
    for(int i = 0; i < udp_table.size; i++) {
        ndn_udp_face_t *face;
        in_port_t port1, port2;
        in_addr_t server_ip;
        char *sz_port1, *sz_port2, *sz_addr;
        uint32_t ul_port;
        struct hostent * host_addr;
        struct in_addr ** paddrs;
        sz_port1 = "1000";
        sz_addr = "0.0.0.0";
        sz_port2 = "1001";
        host_addr = gethostbyname(sz_addr);
        paddrs = (struct in_addr **)host_addr->h_addr_list;
        server_ip = paddrs[0]->s_addr;
        ul_port = strtoul(sz_port1, NULL, 10);
        port1 = htons((uint16_t) ul_port);
        ul_port = strtoul(sz_port2, NULL, 10);
        port2 = htons((uint16_t) ul_port);
        face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);
        udp_table.faces[i] = face;
    }

    char *check_ip, *check_port_1, *check_port_2;
    check_ip = malloc(40);
    check_ip[0] = 0;
    check_port_1 = malloc(40);
    check_port_1[0] = 0;
    check_port_2 = malloc(40);
    check_port_2[0] = 0;
    //char buf[INET_ADDRSTRLEN];

    in_addr_t input;
    int last_face = 0;
    bool found = false;

    for(int i = 0; i < 5; i++) {
        //error because of nothing in the table
        input = udp_table.faces[i]->remote_addr.sin_addr.s_addr;
        inet_ntop(AF_INET, input, check_ip, sizeof(check_ip));
        printf("IP: %s, ", check_ip);
        sprintf(check_port_1, "%d", htons(udp_table.faces[i]->local_addr.sin_port));
        printf("PORT1: %s, ", check_port_1);
        sprintf(check_port_2, "%d", htons(udp_table.faces[i]->remote_addr.sin_port));
        printf("PORT2: %s\n", check_port_2);
    }
    */

    last_interest = ndn_time_now_ms();
    
    //FACE NEEDS TO BE INITIATED WITH CORRECT PARAMETERS BEFORE SENDING OR RECEIVING ANCMT
    //registers ancmt prefix with the forwarder so when ndn_forwarder_process is called, it will call the function on_interest
    //DEMO: CHANGE
    populate_incoming_fib();
    callback_insert(on_data, fill_pit);

    //signature init here

    //DEMO: CHANGE
    //is_anchor = true;
    if(is_anchor == true) {
        send_debug_message("Is Anchor ; ");
    }

    //when production wants to send data and recieve packets, do thread for while loop and thread for sending data when producer wants to
    pthread_create(&forwarding_process_thread, NULL, forwarding_process, NULL);
    pthread_create(&command_process_thread, NULL, command_process, NULL);
    pthread_join(forwarding_process_thread, NULL);
    pthread_join(command_process_thread, NULL);
    // running = true;
    // while (running) {
    //     if(is_anchor && !ancmt_sent) {
    //         //printf("send ancmt called\n");
    //         ndn_interest_t interest;
    //         flood(interest);
    //         ancmt_sent = true;
    //     }
        
    //     ndn_forwarder_process();
    //     usleep(10000);
    // }

    return 0;
}