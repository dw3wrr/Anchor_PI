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

#define PORT 8888
#define DEBUG "155.246.182.138"

int sock = 0;

void send_debug_message(char *input) {
    char *debug_message;
    char buffer[1024] = {0};
    int valread;

    debug_message = input;
    send(sock , debug_message, strlen(debug_message) , 0 );
    //printf("Message Sent\n");
}

int main() {
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
   
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, DEBUG, &serv_addr.sin_addr)<=0) 
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    send_debug_message("Message 1");
    // clock_t start_time = clock();
    // while (clock() < (start_time + 5000000)) {
    // }
    // printf("Delay Complete\n");
    // send_debug_message("Message 2");

    return 0;
}