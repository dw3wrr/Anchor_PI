#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ndn-lite.h>
#include "ndn-lite/encode/name.h"
#include "ndn-lite/encode/data.h"
#include "ndn-lite/encode/interest.h"
#include "ndn-lite/app-support/service-discovery.h"
#include "ndn-lite/app-support/access-control.h"
#include "ndn-lite/app-support/security-bootstrapping.h"
#include "ndn-lite/app-support/ndn-sig-verifier.h"
#include "ndn-lite/app-support/pub-sub.h"
#include "ndn-lite/encode/key-storage.h"
#include "ndn-lite/encode/ndn-rule-storage.h"

//layer 1
//anchor(consumer) acts as a subsciber that sends ancmt (interest packet) to all other basic nodes
//the basic nodes send a data package back with acknowledge

//figure out how to make sure that the ancmt transports to other nodes
//see if it works

in_port_t port1, port2;
in_addr_t server_ip;
ndn_name_t name_prefix;
bool running;

int time_slice = 2;

int
parseArgs(int argc, char *argv[])
{
  char *sz_port1, *sz_port2, *sz_addr;
  uint32_t ul_port;
  struct hostent * host_addr;
  struct in_addr ** paddrs;

  if(argc < 5) {
    fprintf(stderr, "ERROR: wrong arguments.\n");
    printf("Usage: <local-port> <remote-ip> <remote-port> <name-prefix>\n");
    return 1;
  }
  sz_port1 = argv[1];
  sz_addr = argv[2];
  sz_port2 = argv[3];
  //sz_prefix = argv[4];
  //data_need = argv[5];

  if(strlen(sz_port1) <= 0 || strlen(sz_addr) <= 0 || strlen(sz_port2) <= 0){
    fprintf(stderr, "ERROR: wrong arguments.\n");
    return 1;
  }

  host_addr = gethostbyname(sz_addr);
  if(host_addr == NULL){
    fprintf(stderr, "ERROR: wrong hostname.\n");
    return 2;
  }

  paddrs = (struct in_addr **)host_addr->h_addr_list;
  if(paddrs[0] == NULL){
    fprintf(stderr, "ERROR: wrong hostname.\n");
    return 2;
  }
  server_ip = paddrs[0]->s_addr;

  ul_port = strtoul(sz_port1, NULL, 10);
  if(ul_port < 1024 || ul_port >= 65536){
    fprintf(stderr, "ERROR: wrong port number.\n");
    return 3;
  }
  port1 = htons((uint16_t) ul_port);

  ul_port = strtoul(sz_port2, NULL, 10);
  if(ul_port < 1024 || ul_port >= 65536){
    fprintf(stderr, "ERROR: wrong port number.\n");
    return 3;
  }
  port2 = htons((uint16_t) ul_port);

  if(ndn_name_from_string(&name_prefix, argv[4], strlen(argv[4])) != NDN_SUCCESS){
    fprintf(stderr, "ERROR: wrong name.\n");
    return 4;
  }

  return 0;
}

//consumer-side
void
on_data(const uint8_t* rawdata, uint32_t data_size, void* userdata)
{
  ndn_data_t data;
  printf("On data\n");
  if (ndn_data_tlv_decode_digest_verify(&data, rawdata, data_size)) {
    printf("Decoding failed.\n");
  }
  printf("It says: %s\n", data.content_value);
  running = false;
}

void
on_timeout(void* userdata) {
  printf("On timeout\n");
  running = false;
}

void
send_ancmt(ndn_udp_face_t *faceInput, ndn_interest_t interestInput) {
  //change route to multicast
  //on basic node side, the basic node will start as a producer and wait for an interest packet
  //once recieving an interest packet they will check if its name is ancmt
  //producer initalized with ancmt prefix
  printf("Layer 1");

  char *str = "ancmt/data/1";

  ndn_forwarder_add_route_by_name(&faceInput->intf, &name_prefix);
  ndn_interest_from_name(&interestInput, &name_prefix);
  // TODO: Remove recieving data packet
  ndn_forwarder_express_interest_struct(&interestInput, on_data, on_timeout, NULL);
}

int
main(int argc, char *argv[])
{
  ndn_udp_face_t *face;
  ndn_interest_t interest;
  int ret;

  if((ret = parseArgs(argc, argv)) != 0){
    return ret;
  }

  ndn_lite_startup();
  face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);

  if(argv[4] == "ancmt") {
    send_ancmt(face, interest);
  }

  // TODO: divide send and recieve of interest packets
  else {
    ndn_forwarder_add_route_by_name(&face->intf, &name_prefix);
    ndn_interest_from_name(&interest, &name_prefix);
    ndn_forwarder_express_interest_struct(&interest, on_data, on_timeout, NULL);
  }

  running = true;
  while(running) {
    ndn_forwarder_process();
    usleep(10000);
  }

  ndn_face_destroy(&face->intf);
  return 0;
}


/*
int
main(int argc, char *argv[])
{
  ndn_udp_face_t *face;
  ndn_interest_t interest;
  int ret;

  if((ret = parseArgs(argc, argv)) != 0){
    return ret;
  }

  ndn_lite_startup();
  face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);
  if(argv[4] == "ancmt")
    send_ancmt(face, interest);
  }
  else {  
    ndn_forwarder_add_route_by_name(&face->intf, &name_prefix);
    ndn_interest_from_name(&interest, &name_prefix);
    ndn_forwarder_express_interest_struct(&interest, on_data, on_timeout, NULL);
  }

  running = true;
  while(running) {
    ndn_forwarder_process();
    usleep(10000);
  }

  ndn_face_destroy(&face->intf);
  return 0;
}
*/