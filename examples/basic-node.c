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

in_port_t port1, port2;
in_addr_t server_ip;
ndn_name_t name_prefix;
uint8_t buf[4096];
ndn_udp_face_t *face;
bool running;

int
on_ancmt(const uint8_t* interest, uint32_t interest_size, void* userdata)
{
  ndn_data_t data;
  ndn_encoder_t encoder;
  char * str = "Announcement acknoledged";

  printf("On ancmt\n");
  data.name = name_prefix;
  ndn_data_set_content(&data, (uint8_t*)str, strlen(str) + 1);
  ndn_metainfo_init(&data.metainfo);
  ndn_metainfo_set_content_type(&data.metainfo, NDN_CONTENT_TYPE_BLOB);
  encoder_init(&encoder, buf, 4096);
  ndn_data_tlv_encode_digest_sign(&encoder, &data);
  ndn_forwarder_put_data(encoder.output_value, encoder.offset);

  return NDN_FWD_STRATEGY_SUPPRESS;
}

int
on_interest(const uint8_t* interest, uint32_t interest_size, void* userdata)
{
  if(interest.getName() == "ancmt") {
    on_ancmt();
  }
  else {
    ndn_data_t data;
    ndn_encoder_t encoder;
    char * str = "I'm a Data packet.";

    printf("On interest\n");
    data.name = name_prefix;
    ndn_data_set_content(&data, (uint8_t*)str, strlen(str) + 1);
    ndn_metainfo_init(&data.metainfo);
    ndn_metainfo_set_content_type(&data.metainfo, NDN_CONTENT_TYPE_BLOB);
    encoder_init(&encoder, buf, 4096);
    ndn_data_tlv_encode_digest_sign(&encoder, &data);
    ndn_forwarder_put_data(encoder.output_value, encoder.offset);

    return NDN_FWD_STRATEGY_SUPPRESS;
  }
}

int
main(int argc, char *argv[])
{
  

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
