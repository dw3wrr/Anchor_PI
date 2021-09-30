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
#include <sys/socket.h>
#include <arpa/inet.h>
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

#define PORT 8888

struct delay_struct {
    int struct_selector;
    ndn_interest_t interest;
};

ndn_pit_t *layer1_pit;
ndn_fib_t *router_fib;
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

//socket variables
int sock = 0, valread;
struct sockaddr_in serv_addr;
char *debug_message;
char buffer[1024] = {0};


int send_debug_message(char *input) {
    debug_message = input;
    send(sock , debug_message, strlen(debug_message) , 0 );
    valread = read( sock , buffer, 1024);
    printf("%s\n",buffer);
    return 0;
}

void ndn_layer1_flood_ancmt(ndn_interest_t interest) {
    printf("\nFlooding\n");
    ndn_name_t *prefix_name = &interest.name;
    char *prefix = &interest.name.components[0].value[0];
    printf("Prefix: %s\n", prefix);

    router = ndn_forwarder_get();

    //Layer 1 Data Packet
    if(is_anchor) {
        printf("Forwarding Announcement (Anchor)\n");
        ndn_forwarder_express_interest_struct(&interest, NULL, NULL, NULL);
    }

    else {
        printf("Forwarding Announceent (Non-Anchor)\n");
        for(int i = 0; i < router->pit->capacity; i++) {
            printf("Iterate number: %d\n", i);
            ndn_table_id_t temp_pit_id = router->pit->slots[i].nametree_id;
            nametree_entry_t *temp_nametree_entry = ndn_nametree_at(router->nametree, temp_pit_id);
            printf("Here\n");
            ndn_table_id_t temp_fib_id = temp_nametree_entry->fib_id;
            ndn_fib_unregister_face(router->fib, temp_fib_id);
        }
        router->fib = layer1_fib;

        ndn_forwarder_express_interest_struct(&interest, NULL, NULL, NULL);
    }

    printf("Flooded Interest!\n");
    send_debug_message("Flooded Interest");
}


//Send announcement function
void ndn_layer1_send_ancmt() {
    printf("\nSending Announcement...\n");

    ndn_interest_t ancmt;
    ndn_encoder_t encoder;
    ndn_udp_face_t *face;
    ndn_name_t prefix_name;
    char *prefix_string = "/ancmt/1";
    char interest_buf[4096];

    ndn_name_from_string(&prefix_name, prefix_string, strlen(prefix_string));
    ndn_interest_from_name(&ancmt, &prefix_name);

    ndn_time_ms_t timestamp = ndn_time_now_ms();
    
    ndn_interest_set_Parameters(&ancmt, (uint8_t*)&timestamp, sizeof(timestamp));
    ndn_interest_set_Parameters(&ancmt, (uint8_t*)(selector_ptr + 1), sizeof(selector[1]));

    ndn_key_storage_get_empty_ecc_key(&ecc_secp256r1_pub_key, &ecc_secp256r1_prv_key);
    ndn_ecc_make_key(ecc_secp256r1_pub_key, ecc_secp256r1_prv_key, NDN_ECDSA_CURVE_SECP256R1, 890);
    ndn_ecc_prv_init(ecc_secp256r1_prv_key, secp256r1_prv_key_str, sizeof(secp256r1_prv_key_str), NDN_ECDSA_CURVE_SECP256R1, 0);
    ndn_key_storage_t *storage = ndn_key_storage_get_instance();
    ndn_name_t *self_identity_ptr = storage->self_identity;
    ndn_signed_interest_ecdsa_sign(&ancmt, self_identity_ptr, ecc_secp256r1_prv_key);

    flood(ancmt);
    ancmt_sent = true;
    printf("Announcement sent.\n");
    send_debug_message("Announcment Sent");
}

bool ndn_layer1_verify_interest(ndn_interest_t *interest) {
    printf("\nVerifying Packet\n");
    int timestamp = interest->parameters.value[0];
    int current_time = ndn_time_now_ms();

    if((current_time - timestamp) < 0) {
        return false;
    }
    else if(ndn_signed_interest_ecdsa_verify(interest, ecc_secp256r1_pub_key) != NDN_SUCCESS) {
        return false;
    }
    send_debug_message("Interest Verified");
    return true;
}

//ruiran
void reply_ancmt() {
    send_debug_message("Announcent Reply Sent");
}

//ruiran 
void insert_pit(ndn_interest_t interest) {
    send_debug_message("Packet Inserted Into PIT");
}

void *ndn_layer1_start_delay(void *arguments) {
    printf("\nDelay started\n");
    struct delay_struct *args = arguments;
    clock_t start_time = clock();
    printf("Delay Time: %d seconds\n", delay/1000000);
    while (clock() < start_time + delay) {
    }
    printf("Delay Complete\n");
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

int ndn_layer1_interest_receive(const uint8_t* interest, uint32_t interest_size, void* userdata) {
    printf("\nNormal-Node On Interest\n");
    pthread_t layer1;
    ndn_interest_t interest_pkt;
    ndn_interest_from_block(&interest_pkt, interest, interest_size);

    char *prefix = &interest_pkt.name.components[0].value[0];
    prefix = trimwhitespace(prefix);

    printf("PREFIX: %s\n", prefix);

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
    
    if(verify_interest(&interest_pkt) == false) {
        printf("Packet Wrong Format!");
        return NDN_UNSUPPORTED_FORMAT;
    }
    printf("Packet Verified!\n");
    insert_pit(interest_pkt);

    if(strcmp(prefix, "ancmt") == 0 && stored_selectors[parameters] == false) {
        printf("New Ancmt\n");
        stored_selectors[parameters] = true;
        if(delay_start[parameters] != true) {
            pthread_create(&layer1, NULL, &start_delay, (void *)&args);
            delay_start[parameters] = true;
        }
        interface_num[parameters]++;
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
    send_debug_message("On Interest");
    printf("END OF ON_INTEREST\n");
    
    return NDN_FWD_STRATEGY_SUPPRESS;
}

void populate_outgoing_fib() {
    // TODO: make a real populate fib where each node is detected and added into fib
    printf("\nOutgoing FIB populated\n");
    ndn_udp_face_t *face;
    ndn_name_t prefix_name;
    char *ancmt_string = "/ancmt/1";
    ndn_name_from_string(&prefix_name, ancmt_string, strlen(ancmt_string));
    
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
    face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);
    ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);

    router = ndn_forwarder_get();
    //router_fib = router.fib;
}

void populate_incoming_fib() {
    printf("\nIncoming FIB populated\nNOTE: all other nodes must be turned on and in the network, else SegFault \n");
    char *ancmt_string = "/ancmt/1";

    ndn_name_t name_prefix;
    ndn_udp_face_t *face;

    in_port_t port1, port2;
    in_addr_t server_ip;
    char *sz_port1, *sz_port2, *sz_addr;
    uint32_t ul_port;
    struct hostent * host_addr;
    struct in_addr ** paddrs;

    sz_port1 = "5000";
    sz_addr = "rpi1-btran";
    sz_port2 = "3000";
    host_addr = gethostbyname(sz_addr);
    paddrs = (struct in_addr **)host_addr->h_addr_list;
    server_ip = paddrs[0]->s_addr;
    ul_port = strtoul(sz_port1, NULL, 10);
    port1 = htons((uint16_t) ul_port);
    ul_port = strtoul(sz_port2, NULL, 10);
    port2 = htons((uint16_t) ul_port);
    face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);

    sz_port1 = "5000";
    sz_addr = "rpi2-btran";
    sz_port2 = "3000";
    host_addr = gethostbyname(sz_addr);
    paddrs = (struct in_addr **)host_addr->h_addr_list;
    server_ip = paddrs[0]->s_addr;
    ul_port = strtoul(sz_port1, NULL, 10);
    port1 = htons((uint16_t) ul_port);
    ul_port = strtoul(sz_port2, NULL, 10);
    port2 = htons((uint16_t) ul_port);
    face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);

    sz_port1 = "5000";
    sz_addr = "rpi3-btran";
    sz_port2 = "3000";
    host_addr = gethostbyname(sz_addr);
    paddrs = (struct in_addr **)host_addr->h_addr_list;
    server_ip = paddrs[0]->s_addr;
    ul_port = strtoul(sz_port1, NULL, 10);
    port1 = htons((uint16_t) ul_port);
    ul_port = strtoul(sz_port2, NULL, 10);
    port2 = htons((uint16_t) ul_port);
    face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);
    
    ndn_name_from_string(&name_prefix, ancmt_string, strlen(ancmt_string));
    ndn_forwarder_register_name_prefix(&name_prefix, on_interest, NULL);
}


bool verify_data(ndn_data_t *data_pkt, const uint8_t* rawdata, uint32_t data_size) {
    printf("\nVerifying Packet\n");
    //check signature is correct from the public key is valid for all normal nodes
    //check if timestamp is before the current time
    int timestamp = data_pkt->signature.timestamp;
    int current_time = ndn_time_now_ms();
    //verify time slot

    if((current_time - timestamp) < 0) {
        return false;
    }

    else if(ndn_data_tlv_decode_ecdsa_verify(data_pkt, rawdata, data_size, ecc_secp256r1_pub_key) != NDN_SUCCESS) {
        return false;
    }

    send_debug_message("Data Verified");
    return true;
}

void generate_data() {
    //add key, timestamp, selector, vector
    printf("Generate Data");
    ndn_data_t data;
    ndn_encoder_t encoder;
    char *str = "/data/1,2/";
    char *prefix = "/data/1";

    //printf("%s \n", interest);

    data.name = name_prefix;
    ndn_data_set_content(&data, (uint8_t*)str, strlen(str) + 1);
    ndn_metainfo_init(&data.metainfo);
    ndn_metainfo_set_content_type(&data.metainfo, NDN_CONTENT_TYPE_BLOB);
    encoder_init(&encoder, buf, 4096);
    ndn_data_tlv_encode_digest_sign(&encoder, &data);
    ndn_forwarder_put_data(encoder.output_value, encoder.offset);
}

void reply_interest(ndn_data_t *data_pkt, int layer_num) {
    
}

bool check_CS(ndn_data_t *data_pkt) {

}

void on_data(const uint8_t* rawdata, uint32_t data_size, void* userdata) {
    ndn_data_t *data_pkt;
    ndn_data_t *vector;
    char *data_content;
    char *traverse;
    int layer_num;
    uint64_t *timestamp;//uint64_t
    // contentFormat: /data/layerNum/content

    if (ndn_data_tlv_decode_digest_verify(&data_pkt, rawdata, data_size)) {
        printf("Decoding failed\n");
    }

    // if(verify_data(&data_pkt, rawdata, data_size) == false) {
    //     return;
    // }

    data_content = data_pkt->content_value;//uint8
    timestamp = data_pkt->signature->timestamp;

    for(traverse = data_content; *traverse != '\0'; traverse++) {
        if((traverse - '0') == 1 || (traverse - '0') == 2) {
            layer_num = traverse - '0';
        }
    }
    if(layer_num == 1) {
        if(only_normal) {
            reply_interest(data_pkt, 1);
        }
        else {
            reply_interest(data_pkt, 1);
            data_pkt = attaching_vector();
            reply_interest(data_pkt, 2);
        }
    }
    if(layer_num == 2) {
        if(!check_CS(data_pkt)) {
            printf("Check CS fail\n");
        }
        vector = update_vector();
        if() {
            send_update_vector();
        }
        else {
            reply_interest(data_pkt, 2);
        }
    }
 
    printf("It says: %s\n", data_pkt.content_value);
    generate_data();
}

void select_anchor() {

}

int main(int argc, char *argv[]) {
    printf("Main Loop\n");

    //socket connection
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
   
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, "192.168.1.10", &serv_addr.sin_addr)<=0) 
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    send_debug_message("Node Start");
    ndn_lite_startup();
    last_interest = ndn_time_now_ms();

    populate_incoming_fib();

    //is_anchor = true;
    if(is_anchor == true) {
        send_debug_message("Is Anchor");
    }
    running = true;
    while (running) {
        //uncomment here to test send anct
        if(is_anchor && !ancmt_sent) {
            //printf("send anct called\n");
            send_ancmt();
        }
        
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