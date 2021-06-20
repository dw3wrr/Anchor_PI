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
in_addr_t server_ip, local_ip;
ndn_name_t name_prefix;
bool running;
char* prefix_string = "ancmt";
ndn_udp_face_t *face;

void
send_ancmt() {
  //change route to multicast
  //on basic node side, the basic node will start as a producer and wait for an interest packet
  //once recieving an interest packet they will check if its name is ancmt
  //producer initalized with ancmt prefix
  ndn_interest_t interest;

  ndn_name_from_string(&name_prefix, "ancmt", strlen("ancmt"));
  ndn_forwarder_add_route_by_name(&face->intf, &name_prefix);
  ndn_interest_from_name(&interest, &name_prefix);
  ndn_forwarder_express_interest_struct(&interest, on_data, on_timeout, NULL);

}

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

int
main(int argc, char *argv[])
{
  //ndn_udp_face_t *face;
  char *sz_port1, *sz_port2, *sz_addr;
  uint32_t ul_port;
  struct hostent *host_addr;
  struct in_addr **paddrs;

  sz_addr = "192.168.1.6";
  sz_port1 = "3000";
  sz_port2 = "5000";

  host_addr = gethostbyname(sz_addr);
  paddrs = (struct in_addr **)host_addr->h_addr_list;
  server_ip = paddrs[0]->s_addr;
  ul_port = strtoul(sz_port1, NULL, 10);
  port1 = htons((uint16_t) ul_port);
  ul_port = strtoul(sz_port2, NULL, 10);
  port2 = htons((uint16_t) ul_port);

  ndn_lite_startup();
  face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);
  send_ancmt();

  running = true;
  while(running) {
    ndn_forwarder_process();
    usleep(10000);
  }

  ndn_face_destroy(&face->intf);
  return 0;
}
