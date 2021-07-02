#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
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

//To start/stop main loop
bool running;

//To set whether this node is anchor
bool is_anchor = false;

//To set whether an announcement(interest) has been sent
bool ancmt_sent = false;

//Time Slice
int time_slice = 3;

//Selector integers
int selector[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

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

//Send announcement function
void send_ancmt(ndn_interest_t ancmt, ndn_udp_face_t* face) {
    char* prefix_name = "/ancmt/data/1";

    //Sets timestamp
    time_t clk = time(NULL);
    char* timestamp = ctime(&clk);

    printf("Sending Announcement...");
    

    //This is to set up the face to send ancmt (flood)
    //

    //This creates the routes for the interest and sends to nodes
    ndn_forwarder_add_route_by_name(&face->intf, &prefix_name);
    ndn_interest_from_name(&ancmt, &prefix_name);
    //ndn_forwarder_express_interest_struct(&interestInput, on_data, on_timeout, NULL);


    printf("Announcement sent.");
}

void flood() {

}

void on_interest() {

}

void on_data() {

}

int main(int argc, char *argv[]) {
    //ndn_interest_t interest;
    //ndn_udp_face_t *face;
    

    ndn_lite_startup();


    running = true;
    while (running) {
        if(anchor && !ancmt_sent) {
            send_ancmt();
        }
        ndn_forwarder_process();
        usleep(10000);
    }
}