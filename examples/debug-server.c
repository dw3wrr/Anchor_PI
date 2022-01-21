#include <stdio.h> 
#include <string.h>
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>
//#include <graphics.h>
#include <arpa/inet.h> //linux only
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <fenv.h>
     
#define TRUE   1 
#define FALSE  0 
#define PORT 8888

#define NODE1 "155.246.44.142"
#define NODE2 "155.246.215.101"
#define NODE3 "155.246.202.145"
#define NODE4 "155.246.216.113"
#define NODE5 "155.246.203.173"
#define NODE6 "155.246.216.39"
#define NODE7 "155.246.202.111"
#define NODE8 "155.246.212.111"
#define NODE9 "155.246.213.83"
#define NODE10 "155.246.210.98"

struct Node{
    char *data;
    struct Node *firstChild;
    struct Node *nextSibling;
};

/*
void printTree(struct Node *input) {
    struct Node *traverse; //root
    traverse = input;
    if(input == NULL) {
        return;
    }
    printTree(traverse->nextSibling);
    printf("\n");
    printTree(traverse->firstChild);
    printf("%s ", traverse->data);
}

void printNextSibling(struct Node *input) {

}

void printFirstChild(struct Node *input) {

}
*/


void printTree(struct Node *input) {
    struct Node *traverse; //root
    traverse = input;
    //printf("%s\n", traverse->data);
    if(traverse->firstChild == NULL) {
        printf("No Anchor Detected");
    }
    //printf("here\n");
    traverse = traverse->firstChild;
    while(traverse->nextSibling != NULL || traverse->firstChild != NULL)
    {
        printf("%s", traverse->data);
        if(traverse->firstChild != NULL) {
            printf("\n|\n|\n");
            traverse = traverse->firstChild;
        }
        else if(traverse->nextSibling != NULL) {
            printf("---");
            traverse = traverse->nextSibling;
        }
    }
    //final print
    printf("%s", traverse->data);
    printf("\nDone.\n");
    free(traverse);
}


struct Node *addNode(char *input) {
    struct Node *node = (struct Node *)malloc(sizeof(struct Node));
    node->data = input;
    //printf("%s\n", node->data);
    node->firstChild = NULL;
    node->nextSibling = NULL;
    return node;
}
//https://stackoverflow.com/questions/32048392/segmentation-fault-after-returning-a-pointer-to-a-struct

void testTree() {
    struct Node *root = (struct Node *)malloc(sizeof(struct Node));
    root = addNode("root");
    //printf("%s\n", root->data);
    root->firstChild = addNode("192.168.1.10");
    //printf("%s\n", root->firstChild->data);
    root->firstChild->firstChild = addNode("192.168.1.11");
    root->firstChild->firstChild->nextSibling = addNode("192.168.1.12");
    root->firstChild->firstChild->nextSibling->nextSibling = addNode("192.168.1.13");
    root->firstChild->firstChild->nextSibling->nextSibling->firstChild = addNode("192.168.1.14");
    root->firstChild->firstChild->nextSibling->nextSibling->firstChild->nextSibling = addNode("192.168.1.15");
    //printf("TestTree Good\n");
    //debug print tree
    printTree(root);
    free(root);
}

int main(int argc , char *argv[])  
{
    FILE *fp;
    fp = fopen("debug-output.txt", "w+");
    fclose(fp);
    //testTree();
    //init tree with root
    struct Node *root = (struct Node *)malloc(sizeof(struct Node));
    root = addNode("root");

    int opt = TRUE;  
    int master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i , valread , sd;  
    int max_sd;  
    struct sockaddr_in address;  
         
    char buffer[1025];  //data buffer of 1K
    //separate buffer words
         
    //set of socket descriptors
    fd_set readfds;  
         
    //a message 
    char *message = "Now Connected To Debug Server";

    //time variables
    char time_buffer[26];
    int millisec;
    struct tm* tm_info;
    struct timeval tv;

    //initialise all client_socket[] to 0 so not checked 
    for (i = 0; i < max_clients; i++)  
    {  
        client_socket[i] = 0;  
    }  
         
    //create a master socket 
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)  
    {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
     
    //set master socket to allow multiple connections , 
    //this is just a good habit, it will work without this 
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
          sizeof(opt)) < 0 )  
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  
     
    //type of socket created 
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY; //check to see if ndn can address INADDR_ANY
    address.sin_port = htons( PORT );  
         
    //bind the socket to localhost port 8888 
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)  
    {  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    printf("Listener on port %d \n", PORT);  
         
    //try to specify maximum of 3 pending connections for the master socket 
    if (listen(master_socket, 31) < 0)  
    {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  
         
    //accept the incoming connection 
    addrlen = sizeof(address);  
    puts("Waiting for connections ...");  
         
    while(TRUE)  
    {
        //clear the socket set 
        FD_ZERO(&readfds);  
     
        //add master socket to set 
        FD_SET(master_socket, &readfds);  
        max_sd = master_socket;  
             
        //add child sockets to set 
        for ( i = 0 ; i < max_clients ; i++)  
        {  
            //socket descriptor 
            sd = client_socket[i];  
                 
            //if valid socket descriptor then add to read list 
            if(sd > 0)  
                FD_SET( sd , &readfds);  
                 
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  
                max_sd = sd;  
        }  
     
        //wait for an activity on one of the sockets , timeout is NULL , 
        //so wait indefinitely 
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);  
       
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }  
             
        //If something happened on the master socket , 
        //then its an incoming connection 
        if (FD_ISSET(master_socket, &readfds))  
        {  
            if ((new_socket = accept(master_socket, 
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  
            {  
                perror("accept");  
                exit(EXIT_FAILURE);  
            }  
             
            //inform user of socket number - used in send and receive commands
            char *node_num = "Node";
            if(strcmp( inet_ntoa(address.sin_addr), NODE1) == 0) {
                node_num = "Node 1";
            }
            else if(strcmp( inet_ntoa(address.sin_addr), NODE2) == 0) {
                node_num = "Node 2";
            }
            else if(strcmp( inet_ntoa(address.sin_addr), NODE3) == 0) {
                node_num = "Node 3";
            }
            else if(strcmp( inet_ntoa(address.sin_addr), NODE4) == 0) {
                node_num = "Node 4";
            }
            else if(strcmp( inet_ntoa(address.sin_addr), NODE5) == 0) {
                node_num = "Node 5";
            }
            else if(strcmp( inet_ntoa(address.sin_addr), NODE6) == 0) {
                node_num = "Node 6";
            }
            else if(strcmp( inet_ntoa(address.sin_addr), NODE7) == 0) {
                node_num = "Node 7";
            }
            else if(strcmp( inet_ntoa(address.sin_addr), NODE8) == 0) {
                node_num = "Node 8";
            }
            else if(strcmp( inet_ntoa(address.sin_addr), NODE9) == 0) {
                node_num = "Node 9";
            }
            else if(strcmp( inet_ntoa(address.sin_addr), NODE10) == 0) {
                node_num = "Node 10";
            }
            printf("New connection , socket fd is %d , ip is : %s , port : %d (%s)\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port), node_num);

           
            //send new connection greeting message 
            /*
            if( send(new_socket, message, strlen(message), 0) != strlen(message) )  
            {  
                perror("send");  
            }  
                 
            puts("Welcome message sent successfully");  
            */
                 
            //add new socket to array of sockets 
            for (i = 0; i < max_clients; i++)  
            {  
                //if position is empty 
                if( client_socket[i] == 0 )  
                {  
                    client_socket[i] = new_socket;  
                    printf("Adding to list of sockets as %d\n" , i);  
                         
                    break;  
                }  
            }  
        }  
             
        //else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++)  
        {  
            sd = client_socket[i];  
                 
            if (FD_ISSET( sd , &readfds))  
            {  
                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = read( sd , buffer, 1024)) == 0)  
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);  
                    printf("Host disconnected , ip %s , port %d \n" , 
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                         
                    //Close the socket and mark as 0 in list for reuse 
                    close( sd );  
                    client_socket[i] = 0;  
                }  
                     
                //Echo back the message that came in 
                else 
                {  
                    fp = fopen("debug-output.txt", "a+");
                    //set the string terminating NULL byte on the end 
                    //of the data read

                    gettimeofday(&tv, NULL);

                    millisec = tv.tv_usec/1000.0; // Round to nearest millisec
                    if (millisec>=1000) { // Allow for rounding up to nearest second
                        millisec -=1000;
                        tv.tv_sec++;
                    }

                    tm_info = localtime(&tv.tv_sec);

                    strftime(time_buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
                    printf("%s.%03d ", time_buffer, millisec);

                    //here add recording information about incoming message that is not a new connection
                    //so what type: ancmt send/ancmt receive/
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);
                    char *temp = inet_ntoa(address.sin_addr);
                    char *node_num = "Node";
                    if(strcmp(temp, NODE1) == 0) {
                        node_num = "Node 1";
                    }
                    else if(strcmp(temp, NODE2) == 0) {
                        node_num = "Node 2";
                    }
                    else if(strcmp(temp, NODE3) == 0) {
                        node_num = "Node 3";
                    }
                    else if(strcmp(temp, NODE4) == 0) {
                        node_num = "Node 4";
                    }
                    else if(strcmp(temp, NODE5) == 0) {
                        node_num = "Node 5";
                    }
                    else if(strcmp(temp, NODE6) == 0) {
                        node_num = "Node 6";
                    }
                    else if(strcmp(temp, NODE7) == 0) {
                        node_num = "Node 7";
                    }
                    else if(strcmp(temp, NODE8) == 0) {
                        node_num = "Node 8";
                    }
                    else if(strcmp(temp, NODE9) == 0) {
                        node_num = "Node 9";
                    }
                    else if(strcmp(temp, NODE10) == 0) {
                        node_num = "Node 10";
                    }
                    printf("%s: -> ", node_num);
                    //printf("IP ADDRESS: %s -> ", temp);
                    fprintf(fp, "IP ADDRESS: %s -> ", temp);

                    /*
                    struct Node *tempNode;
                    tempNode.data = temp;
                    if(buffer[valread] == "Is Anchor") {
                        root.firstChild = tempNode;
                    }
                    else if(buffer[valread] == "Announcment Sent") {

                    }
                    else if(buffer[valread] == "On Interest") {
                        if(root.firstChild->firstChild == NULL) {
                            root.firstChild->firstChild = tempNode;
                        }
                        else{
                            //most recent addition is the first child of anchor, so oldest is at end of traverse;
                            tempNode->nextSibling = root.firstChild->firstChild->nextSibling;
                            root.firstChild->firstChild = tempNode;
                        }
                    }
                    */
                    buffer[valread] = '\0';
                    char* debug_message = buffer;
                    //printf("VALREAD: %d\n", valread);
                    printf("MESSAGE: %s\n", debug_message);
                    fprintf(fp, "MESSAGE: %s\n", debug_message);
                    //send(sd , buffer , strlen(buffer) , 0 );
                    fclose(fp);
                }  
            }  
        }  
    }
    free(root);
    fclose(fp);
         
    return 0;
}