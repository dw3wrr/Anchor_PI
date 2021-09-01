#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <setjmp.h>
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

//in the build directory go to make files and normal node -change the link.txt
//CMAKE again
//then make
//link.txt
///usr/bin/cc  -std=c11 -Werror -Wno-format -Wno-int-to-void-pointer-cast -Wno-int-to-pointer-cast -O3   CMakeFiles/normal-node.dir/examples/normal-node.c.o  -pthread -o examples/normal-node  libndn-lite.a

struct delay_struct {
    int struct_selector;
    ndn_interest_t interest;
};

//intitialize pit and fib for layer 1
ndn_pit_t *layer1_pit;
ndn_fib_t *layer1_fib;
ndn_forwarder_t *router;
//char ip_address = "192.168.1.10";

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
uint8_t selector[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; //change from 0 to 9
uint8_t *selector_ptr = selector;
bool stored_selectors[10];

bool delay_start[10];
//clock time is in nano seconds, divide by 10^6 for actual time
int delay = 3000000;
int max_interfaces = 8;
//set array for multiple anchors for anchor/selector 1 - 10
int interface_num[10];
bool did_flood[10];

//
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

//may have to use interest as a pointer
void flood(ndn_interest_t interest) {
    printf("\nFlooding\n");
    //multithread: while in time delay period keep accepting other announcements
    //ndn_udp_face_t face;
    ndn_name_t *prefix_name = &interest.name;
    //printf("%s\n", prefix_name);
    char *prefix = &interest.name.components[0].value[0];
    printf("Prefix: %s\n", prefix);
    
    //gets the forwarder intiailized in the main message
    //router_const = ndn_forwarder_get();
    router = ndn_forwarder_get();

    //Layer 1 Data Packet
    if(is_anchor) {
        //Anchor flooding announcement (layer 1)
        //Flood without accounting for time delay or max number of interfaces
        //Get all closest interfaces and forward to them
        printf("Forwarding Announcement (Anchor)\n");
        //insert fib information here
        ndn_forwarder_express_interest_struct(&interest, NULL, NULL, NULL);

        // for(int i = 0; i < ndn_forwarder_get().fib.capacity; i ++) {
        //     //printf("looking at interfaces in fib")
        //     ndn_forwarder_express_interest_struct(&interest, on_data, NULL, NULL);
        // }
    }

    else {
        //Normal node flooding announcement (layer 1)
        //Flood while using time delay and accounting for interfaces
        //check pit for incoming interest, then send out interest for each not in pit
        //layer1_fib = router->fib;
        printf("Forwarding Announceent (Non-Anchor)\n");
        //printf("PIT Entires: %d\n", sizeof(router->pit->slots));
        // for(int i = 0; i < router->pit->capacity; i++) {
        //     printf("Iterate number: %d\n", i);
        //     ndn_table_id_t temp_pit_id = router->pit->slots[i].nametree_id;
        //     nametree_entry_t *temp_nametree_entry = ndn_nametree_at(router->nametree, temp_pit_id);
        //     printf("Here\n");
        //     ndn_table_id_t temp_fib_id = temp_nametree_entry->fib_id;
        //     //TODO: Segmentation Fault Here
        //     ndn_fib_unregister_face(router->fib, temp_fib_id);
        // }
        //router->fib = layer1_fib;

        ndn_forwarder_express_interest_struct(&interest, NULL, NULL, NULL);

        // for(int i = 0; i < layer1_fib.capacity; i++) {
        //     ndn_forwarder_express_interest_struct(&interest, on_data, NULL, NULL);
        // }
    }

    printf("Flooded Interest!\n");
}


//Send announcement function
void send_ancmt() {
    printf("\nSending Announcement...\n");

    //include periodic subscribe of send_anct
    //ndn_interest_t *ancmt = new ndn_interest_t();
    //malloc
    ndn_interest_t ancmt;
    ndn_encoder_t encoder;
    ndn_udp_face_t *face;
    ndn_name_t prefix_name;
    char *prefix_string = "/ancmt/1";
    char interest_buf[4096];

    //. instead ->, initialize as a pointer object first, testing new keyword

    //Sets timestamp
    //time_t clk = time(NULL);
    //char* timestamp = ctime(&clk);

    //This creates the routes for the interest and sends to nodes
    //ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);
    ndn_name_from_string(&prefix_name, prefix_string, strlen(prefix_string));
    //FIXED TODO: Segmentation Fault Here
    ndn_interest_from_name(&ancmt, &prefix_name);
    //ndn_forwarder_express_interest_struct(&interest, on_data, on_timeout, NULL);

    //gets ndn (timestamp)
    ndn_time_ms_t timestamp = ndn_time_now_ms();

    //parameter may be one whole string so the parameters may have to be sorted and stored in a way that is readabel by other normal nodes
    //Init ancmt with selector, signature, and timestamp
    //may have to use ex: (uint8_t*)str for middle param
    
    ndn_interest_set_Parameters(&ancmt, (uint8_t*)&timestamp, sizeof(timestamp));
    ndn_interest_set_Parameters(&ancmt, (uint8_t*)(selector_ptr + 1), sizeof(selector[1]));
    //ndn_interest_set_Parameters(&ancmt, (uint8_t*)ip_address, sizeof(ip_address));

    //Signed interest init
    ndn_key_storage_get_empty_ecc_key(&ecc_secp256r1_pub_key, &ecc_secp256r1_prv_key);
    ndn_ecc_make_key(ecc_secp256r1_pub_key, ecc_secp256r1_prv_key, NDN_ECDSA_CURVE_SECP256R1, 890);
    ndn_ecc_prv_init(ecc_secp256r1_prv_key, secp256r1_prv_key_str, sizeof(secp256r1_prv_key_str), NDN_ECDSA_CURVE_SECP256R1, 0);
    ndn_key_storage_t *storage = ndn_key_storage_get_instance();
    ndn_name_t *self_identity_ptr = storage->self_identity;
    //FIXED TODO: segmentation fault here
    ndn_signed_interest_ecdsa_sign(&ancmt, self_identity_ptr, ecc_secp256r1_prv_key);
    encoder_init(&encoder, interest_buf, 4096);
    //FIXED TODO: Segmentation Fault Here
    ndn_interest_tlv_encode(&encoder, &ancmt);

    //uncomment here to test flood
    flood(ancmt);
    ancmt_sent = true;
    printf("Announcement sent.\n");
}

//how do i populate the fib??????????
//how do i populate the pit
//how do you send an interest to set of given entries inside pit of fib

void populate_fib() {
    // TODO: make a real populate fib where each node is detected and added into fib
    printf("\nFIB populated\n");
    ndn_udp_face_t *face;
    ndn_name_t prefix_name;
    char *ancmt_string = "/ancmt/1";
    
    //myip, my outgoing port, their incoming ip, their incoming port
    
    in_port_t port1, port2;
    in_addr_t server_ip;
    char *sz_port1, *sz_port2, *sz_addr;
    uint32_t ul_port;
    struct hostent * host_addr;
    struct in_addr ** paddrs;

    //pi1->pi2: 192.168.1.10
    sz_port1 = "3000";
    sz_addr = "rpi2-btran";
    sz_port2 = "5000";
    host_addr = gethostbyname(sz_addr);
    paddrs = (struct in_addr **)host_addr->h_addr_list;
    server_ip = paddrs[0]->s_addr;
    ul_port = strtoul(sz_port1, NULL, 10);
    port1 = htons((uint16_t) ul_port);
    ul_port = strtoul(sz_port2, NULL, 10);
    port2 = htons((uint16_t) ul_port);
    ndn_name_from_string(&prefix_name, ancmt_string, strlen(ancmt_string));
    face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);
    ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);

    //pi2->pi3: 192.168.1.11
    sz_port1 = "3000";
    sz_addr = "rpi3-btran";
    sz_port2 = "5000";
    host_addr = gethostbyname(sz_addr);
    paddrs = (struct in_addr **)host_addr->h_addr_list;
    server_ip = paddrs[0]->s_addr;
    ul_port = strtoul(sz_port1, NULL, 10);
    port1 = htons((uint16_t) ul_port);
    ul_port = strtoul(sz_port2, NULL, 10);
    port2 = htons((uint16_t) ul_port);
    ndn_name_from_string(&prefix_name, ancmt_string, strlen(ancmt_string));
    face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);
    ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);
}

bool verify_packet(ndn_interest_t *interest) {
    printf("\nVerifying Packet\n");
    //check signature is correct from the public key is valid for all normal nodes
    //check if timestamp is before the current time
    int timestamp = interest->parameters.value[0];
    int current_time = ndn_time_now_ms();
    //verify time slot

    if((current_time - timestamp) < 0) {
        return false;
    }
    else if(ndn_signed_interest_ecdsa_verify(interest, ecc_secp256r1_pub_key) != NDN_SUCCESS) {
        return false;
    }
    return true;
}

//ruiran
void reply_ancmt() {

}

//ruiran 
void insert_pit(ndn_interest_t interest) {

}

void *start_delay(void *arguments) {
    printf("\nDelay started\n");
    struct delay_struct *args = arguments;
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
        reply_ancmt();
        pthread_exit(NULL);
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

int on_interest(const uint8_t* interest, uint32_t interest_size, void* userdata) {
    printf("\nNormal-Node On Interest\n");
    pthread_t layer1;
    ndn_interest_t interest_pkt;
    ndn_interest_from_block(&interest_pkt, interest, interest_size);

    char *prefix = &interest_pkt.name.components[0].value[0];
    prefix = trimwhitespace(prefix);
    // char prefix_convert[10];
    // int j = sprintf(prefix_convert, "%s", prefix);
    // char *prefix_check = "ancmt";
    printf("PREFIX: %s\n", prefix);
    // printf("PREFIX_CONVERT: `%s`\n", prefix_convert);
    // printf("PREFIX_CHECK: `%s`\n", prefix_check);
    // printf("PREFIX(int): %d\n", prefix);
    // printf("PREFIX CONVERT(int): %d\n", prefix_convert);
    // printf("PREFIX_CHECK(int): %d\n", prefix_check);
    

    
    // for (int i = 0; i < interest_pkt.name.components_size; i++) {
    //     printf("/");
    //     for (int j = 0; j < interest_pkt.name.components[i].size; j++) {
    //         printf("%c", interest_pkt.name.components[i].value[j]);
    //     }
    // }
    // printf("\n");

    int timestamp = interest_pkt.parameters.value[0];
    printf("TIMESTAMP: %d\n", timestamp);
    int current_time = ndn_time_now_ms();
    printf("LAST INTEREST: %d\n", last_interest);
    
    //selector number
    int parameters = interest_pkt.parameters.value[0];
    printf("SELECTOR: %d\n", parameters);
    printf("STORED SELECTOR: %d\n", stored_selectors[parameters]);

    struct delay_struct args;
    args.interest = interest_pkt;
    args.struct_selector = parameters;
    
    //printf("%s\n", prefix);

    //make sure to uncomment verify 
    // if(verify_packet(&interest_pkt) == false) {
    //     printf("Packet Wrong Format!");
    //     return NDN_UNSUPPORTED_FORMAT;
    // }
    //printf("Packet Verified!\n");
    insert_pit(interest_pkt);

    // if(strcmp(prefix, "ancmt") == 0) {
    //     printf("Prefix good\n");
    // }
    // if(strcmp(prefix, prefix_check) == 0) {
    //     printf("Prefix good(var)\n");
    // }
    // if(stored_selectors[parameters] == false) {
    //     printf("Selector false\n");
    // }
    // if(stored_selectors[parameters] == true) {
    //     printf("Selector true\n");
    // }

    //check ancmt, stored selectors, and timestamp(maybe)
    //timestamp + selector for new and old
    //TODO: fix time to coorespond to last ancmt timestamp
    //if((prefix == "ancmt") && stored_selectors[parameters] == false && (timestamp - last_interest) > 0) {
    if(strcmp(prefix, "ancmt") == 0 && stored_selectors[parameters] == false) {
        printf("New Ancmt\n");
        stored_selectors[parameters] = true;
        if(delay_start[parameters] != true) {
            pthread_create(&layer1, NULL, &start_delay, (void *)&args);
            delay_start[parameters] = true;
        }
        interface_num[parameters]++;
        // if(interface_num[parameters] >= max_interfaces) {
        //    flood(interest_pkt);
        //    did_flood[parameters] = true;
        //    reply_ancmt();
        // }    
    }

    else if(strcmp(prefix, "ancmt") == 0 && stored_selectors[parameters] == true) {
        printf("Old Ancmt\n");
        interface_num[parameters]++;
        if(interface_num[parameters] >= max_interfaces) {
            if(did_flood[parameters] == true) {
            }
            else {
                flood(interest_pkt);
                did_flood[parameters] = true;
                reply_ancmt();
                pthread_exit(NULL);
            }
        }
    }

    last_interest = current_time;
    printf("END OF ON_INTEREST\n");
    
    return NDN_FWD_STRATEGY_SUPPRESS;
}
/*
//
void reply_interest(ndn_data_t *data, int layer_num) {

}

//send without ancmt 
void generate_data() {
    //check from layer 1 PIT to reply data packet
    ndn_data_t data_pkt;
    ndn_encoder_t encoder;
    ndn_name_t *name_prefix;
    char *str = "/data/1";

    ndn_name_from_string(&name_prefix, &str, strlen(str));
    data.name = name_prefix;
    str = "Layer 1 Data Packet";
    ndn_time_ms_t timestamp = ndn_time_now_ms();
    uint8_t *data_content = (uint8_t)str + (uint8_t)timestamp;

    ndn_data_set_content(&data_pkt, (uint8_t*)data_content, sizeof(data_content));
    ndn_metainfo_init(&data_pkt.metainfo);
    ndn_metainfo_set_content_type(&data_pkt.metainfo, NDN_CONTENT_TYPE_BLOB);

    encoder_init(&encoder, buf, 4096);
    ndn_data_tlv_encode_ecdsa_sign(&encoder, &data_pkt, &name_prefix, &ecc_secp256r1_prv_key);
    ndn_forwarder_put_data(encoder.output_value, encoder.offset);
}

void verify_data(ndn_data_t *data) {
    ndn_data_tlv_decode_ecdsa_verify(ndn_data_t* data, const uint8_t* block_value, uint32_t block_size, const ndn_ecc_pub_t* pub_key);
}

void on_data(const uint8_t* rawdata, uint32_t data_size, void* userdata) {
    ndn_data_t *data_pkt;

    if(verify_data(&data_pkt) == false) {
        return;
    }

    //printf("On data\n");
    if (ndn_data_tlv_decode_digest_verify(&data_pkt, rawdata, data_size)) {
        printf("Decoding failed.\n");
    }

    printf("It says: %s\n", data_pkt.content_value);
}

//debug pit
void debug_ndn() {
    //get pit data
    //send to http website
    //whenever any new pit entries come in send to http
    ndn_pit_t debug_pit;
    ndn_fib_t debug_fib;
}
*/

int main(int argc, char *argv[]) {
    printf("Main Loop\n");
    ndn_lite_startup();
    //ndn_interest_t interest;
    //pthread_t layer1;
    char *ancmt_string = "/ancmt/1";

    // in_port_t port1, port2;
    // in_addr_t server_ip;
    ndn_name_t name_prefix;
    // ndn_udp_face_t *face;
    // char *sz_port1, *sz_port2, *sz_addr;
    // uint32_t ul_port;
    // struct hostent * host_addr;
    // struct in_addr ** paddrs;

    // sz_port1 = "5000";
    // sz_addr = "rpi3-btran";
    // sz_port2 = "3000";

    // host_addr = gethostbyname(sz_addr);
    // paddrs = (struct in_addr **)host_addr->h_addr_list;
    // server_ip = paddrs[0]->s_addr;
    // ul_port = strtoul(sz_port1, NULL, 10);
    // port1 = htons((uint16_t) ul_port);
    // ul_port = strtoul(sz_port2, NULL, 10);
    // port2 = htons((uint16_t) ul_port);

    ndn_name_from_string(&name_prefix, ancmt_string, strlen(ancmt_string));
    //face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);
    ndn_forwarder_register_name_prefix(&name_prefix, on_interest, NULL);
    last_interest = ndn_time_now_ms();
    
    //FACE NEEDS TO BE INITIATED WITH CORRECT PARAMETERS BEFORE SENDING OR RECEIVING ANCMT
    //populate_fib();
    //registers ancmt prefix with the forwarder so when ndn_forwarder_process is called, it will call the function on_interest

    //signature init

    //is_anchor = true;
    running = true;
    while (running) {
        //printf("nope");
        //uncomment here to test send anct
        if(is_anchor && !ancmt_sent) {
            //printf("send anct called\n");
            send_ancmt();
        }
        //packet is ancmt
        
        // if(ndn_forwarder_receive(ndn_face_intf_t* face, uint8_t* packet, size_t length) == NDN_SUCCESS) {
        //     if(delay_start != true) {
        //         pthread_create(&layer1, NULL, start_delay, delay);
        //         delay_start = true;
        //     }
        //     pthread_create(&layer1, NULL, on_interest, NULL);
        //     interface_num++;
        //     if(interface_num >= max_interfaces) {
        //         flood();
        //     }
        // }
        
        ndn_forwarder_process();
        usleep(10000);
    }
    //ndn_face_destroy(&face->intf);
}

//test main method
/*
int main() {
    ndn_name_t prefix_name;
    char *ancmt_string = "/ancmt";
    

    ndn_lite_startup();
    ndn_name_from_string(&prefix_name, ancmt_string, strlen(ancmt_string));
    
    ndn_key_storage_get_empty_ecc_key(&ecc_secp256r1_pub_key, &ecc_secp256r1_prv_key);
    ndn_ecc_make_key(ecc_secp256r1_pub_key, ecc_secp256r1_prv_key, NDN_ECDSA_CURVE_SECP256R1, 890);
    ndn_ecc_prv_init(ecc_secp256r1_prv_key, secp256r1_prv_key_str, sizeof(secp256r1_prv_key_str), NDN_ECDSA_CURVE_SECP256R1, 0);
    storage = ndn_key_storage_get_instance();

    ndn_interest_t lol;
    ndn_interest_from_name(&lol, &prefix_name);
    //is_anchor = true;
    flood(lol);

    running = true;
    while (running) {
        ndn_forwarder_process();
        usleep(10000);
    }
}
*/