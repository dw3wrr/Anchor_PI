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

char return_string[80] = "";

char *get_prefix_component(ndn_name_t input_name, int num_input) {
    printf("Get Prefix Component %d\n",num_input);
    memset(return_string, 0, sizeof(return_string));
    ndn_name_t prefix_name;
    prefix_name = input_name;
    printf("here\n");
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

int main() {
    ndn_interest_t interest;
    ndn_name_t prefix_name;
    //DEMO: CHANGE
    char *ancmt_string = "/ancmt/1/1";

    ndn_name_from_string(&prefix_name, ancmt_string, strlen(ancmt_string));

    // printf("Components Size: %d", prefix_name.components_size);

    // printf("'");
    // for (int i = 0; i < prefix_name.components_size; i++) {
    //     printf("/");
    //     for (int j = 0; j < prefix_name.components[i].size; j++) {
    //         if (prefix_name.components[i].value[j] >= 33 && prefix_name.components[i].value[j] < 126) {
    //             printf("%c", prefix_name.components[i].value[j]);
    //         }
    //         // else {
    //         //     printf("0x%02x", component.value[j]);
    //         // }
    //     }
    // }
    // printf("'\n'");
    // ndn_name_print(&prefix_name);
    // printf("'");

    char *test = "";
    test = get_prefix_component(prefix_name, 0);
    printf("test component: %s\n", test);
    
    return 0;
}

// ndn_name_print(const ndn_name_t* name)
// {
//   for (int i = 0; i < name->components_size; i++) {
//     name_component_print(&name->components[i]);
//   }
//   printf("\n");
// }

// name_component_print(const name_component_t* component)
// {
//   switch (component->type) {
//     case TLV_ImplicitSha256DigestComponent:
//       printf("/sha256digiest=0x");
//       for (int j = 0; j < component->size; j++) {
//         printf("%02x", component->value[j]);
//       }
//       break;

//     case TLV_ParametersSha256DigestComponent:
//       printf("/params-sha256=0x");
//       for (int j = 0; j < component->size; j++) {
//         printf("%02x", component->value[j]);
//       }
//       break;

//     case TLV_VersionNameComponent:
//       printf("/v=%" PRIu64 "", name_component_to_version(component));
//       break;

//     case TLV_TimestampNameComponent:
//       printf("/t=%" PRI_ndn_time_us_t "", name_component_to_timestamp(component));
//       break;

//     case TLV_SequenceNumNameComponent:
//       printf("/seq=%" PRIu64 "", name_component_to_sequence_num(component));
//       break;

//     default:
//       printf("/");
//       for (int j = 0; j < component->size; j++) {
//         if (component->value[j] >= 33 && component->value[j] < 126) {
//           printf("%c", component->value[j]);
//         }
//         else {
//           printf("0x%02x", component->value[j]);
//         }
//       }
//       break;
//   }
// }