/*
 * Copyright (C) 2019 Xinyu Ma, Zhiyi Zhang
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v3.0. See the file LICENSE in the top level
 * directory for more details.
 *
 * See AUTHORS.md for complete list of NDN IOT PKG authors and contributors.
 */
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ndn-lite.h>
#include "../adaptation/udp/udp-face.h"
#include "ndn-lite/forwarder/forwarder.h"
#include "ndn-lite/encode/data.h"
#include "ndn-lite/encode/interest.h"

in_port_t port;
in_addr_t multicast_ip;
ndn_name_t name_prefix;
uint8_t buf[4096];
ndn_udp_face_t *face;
bool running;

int
parseArgs(int argc, char *argv[])
{
  char *sz_port, *sz_addr;
  uint32_t ul_port;

  if (argc < 2) {
    fprintf(stderr, "ERROR: wrong arguments.\n");
    printf("Usage: <name-prefix> <port>=56363 <group-ip>=224.0.23.170\n");
    return 1;
  }
  if (ndn_name_from_string(&name_prefix, argv[1], strlen(argv[1])) != NDN_SUCCESS) {
    fprintf(stderr, "ERROR: wrong name.\n");
    return 4;
  }
  uint32_t portnum = 56363;
  port = htons(portnum);
  multicast_ip = inet_addr("192.168.1.28");
  if (argc >= 3) {
    sz_port = argv[2];
    if (strlen(sz_port) <= 0) {
      fprintf(stderr, "ERROR: wrong arguments.\n");
      return 1;
    }
    ul_port = strtoul(sz_port, NULL, 10);
    if (ul_port < 1024 || ul_port >= 65536) {
      fprintf(stderr, "ERROR: wrong port number.\n");
      return 3;
    }
    port = htons((uint16_t) ul_port);
  }
  if (argc >= 4) {
    sz_addr = argv[3];
    if (strlen(sz_addr) <= 0) {
      fprintf(stderr, "ERROR: wrong arguments.\n");
      return 1;
    }
    multicast_ip = inet_addr(sz_addr);
  }
  return 0;
}

int
on_interest(const uint8_t* interest, uint32_t interest_size, void* userdata)
{
  ndn_interest_t interest_pkt;
  ndn_interest_from_block(&interest_pkt, interest, interest_size);
  ndn_data_t data;
  ndn_encoder_t encoder;
  char * str = "I'm a Data packet.";

  printf("On interest\n");
  data.name = interest_pkt.name;
  ndn_data_set_content(&data, (uint8_t*)str, strlen(str));
  ndn_metainfo_init(&data.metainfo);
  ndn_metainfo_set_content_type(&data.metainfo, NDN_CONTENT_TYPE_BLOB);
  encoder_init(&encoder, buf, 4096);
  ndn_data_tlv_encode_digest_sign(&encoder, &data);
  ndn_forwarder_put_data(encoder.output_value, encoder.offset);

  return NDN_FWD_STRATEGY_SUPPRESS;
}

int
main(int argc, char *argv[])
{
  printf("Running main...\n");
  int ret;
  if ((ret = parseArgs(argc, argv)) != 0) {
    return ret;
  }

  printf("Running startup...\n");
  ndn_lite_startup();
  face = ndn_udp_multicast_face_construct(INADDR_ANY, multicast_ip, port);
  ndn_forwarder_register_name_prefix(&name_prefix, on_interest, NULL);

  printf("Running forwarder...\n");
  running = true;
  while (running) {
    ndn_forwarder_process();
    usleep(10000);
  }
  ndn_face_destroy(&face->intf);
  return 0;
}