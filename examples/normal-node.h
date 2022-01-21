#ifndef NORMAL_NODE_H
#define NORMAL_NODE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

//void on_data(const uint8_t* rawdata, uint32_t data_size);

#ifdef __cplusplus
}
#endif

#endif

//implement this so that we can use data inside normal node when calling on_data inside forwarder
//currently hardcoded into forwarder.c ndn_forwarder_receive()