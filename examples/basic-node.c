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
//basic node acts as a producer inlayer 1
//when the node receives an ancmt (interest) packet it sends an acknowledgeent packet back
//

//Anchor Parameters:
int time_slice = 2;
char selectors[];



in_port_t port1, port2;
in_addr_t server_ip;
ndn_name_t name_prefix;
uint8_t buf[4096];
ndn_udp_face_t *face;
bool running;

int
parseArgs(int argc, char *argv[])
{
  char *sz_port1, *sz_port2, *sz_addr;
  uint32_t ul_port;
  struct hostent * host_addr;
  struct in_addr ** paddrs;

  if (argc < 5) {
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

//Producers
int
on_interest(const uint8_t* interest, uint32_t interest_size, void* userdata)
{
  ndn_data_t data;
  ndn_encoder_t encoder;
  char *str;
  printf("On interest\n");
  //printf("%s\n", interest);

  ndn_interest_t interest_pkt;
  ndn_interest_from_block(&interest_pkt, interest, interest_size);
  //ndn_name_print(&interest_pkt.name);
  char *prefix = &interest_pkt.name.components[0].value[0];
  printf("%s\n", prefix);

  //Layer 1 Data Packet
  if(prefix = "ancmt") {
    str = "Announcement acknoledged.";
  }

  else {
    str = "I'm a Data packet.";
  }

  // TODO: move to a new function
  data.name = name_prefix;
  ndn_data_set_content(&data, (uint8_t*)str, strlen(str) + 1);
  ndn_metainfo_init(&data.metainfo);
  ndn_metainfo_set_content_type(&data.metainfo, NDN_CONTENT_TYPE_BLOB);
  encoder_init(&encoder, buf, 4096);
  ndn_data_tlv_encode_digest_sign(&encoder, &data);
  ndn_forwarder_put_data(encoder.output_value, encoder.offset);

  return NDN_FWD_STRATEGY_SUPPRESS;
}

//Reply data packet
int
reply_ancmt()
{
  ndn_data_t data;
  // data/2/
}

int
main(int argc, char *argv[])
{
  int ret;
  if ((ret = parseArgs(argc, argv)) != 0) {
    return ret;
  }

  ndn_lite_startup();
  face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);
  ndn_forwarder_register_name_prefix(&name_prefix, on_interest, NULL);

  running = true;
  while (running) {
    ndn_forwarder_process();
    usleep(10000);
  }

  ndn_face_destroy(&face->intf);
  return 0;
}

/* TODO: Split reciveing ancmt functon and sending reply

*/

/*
int
main(int argc, char *argv[])
{
  char *sz_port1, *sz_port2, *sz_addr;
  uint32_t ul_port;
  struct hostent *host_addr;
  struct in_addr **paddrs;

  sz_addr = "192.168.1.3";
  sz_port1 = "5000";
  sz_port2 = "3000";

  host_addr = gethostbyname(sz_addr);
  paddrs = (struct in_addr **)host_addr->h_addr_list;
  server_ip = paddrs[0]->s_addr;
  ul_port = strtoul(sz_port1, NULL, 10);
  port1 = htons((uint16_t) ul_port);
  ul_port = strtoul(sz_port2, NULL, 10);
  port2 = htons((uint16_t) ul_port);
  ndn_name_from_string(&name_prefix, "test", strlen("test"));

  ndn_lite_startup();
  face = ndn_udp_unicast_face_construct(INADDR_ANY, port1, server_ip, port2);
  ndn_forwarder_register_name_prefix(&name_prefix, on_interest, NULL);

  running = true;
  while (running) {
    ndn_forwarder_process();
    usleep(10000);
  }

  ndn_face_destroy(&face->intf);
  return 0;
}
*/
