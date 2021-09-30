#include <stdio.h> 
#include <string.h>
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>
//#include <graphics.h>
#include <arpa/inet.h> //linux only
//#include <Winsock2.h> //windows only
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h>
     
#define TRUE   1 
#define FALSE  0 
#define PORT 8888

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
    testTree();
    /*
    struct Node root;
    root = addNode("root");
    int opt = TRUE;  
    int master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i , valread , sd;  
    int max_sd;  
    struct sockaddr_in address;  
         
    char buffer[1025];  //data buffer of 1K 
         
    //set of socket descriptors
    fd_set readfds;  
         
    //a message 
    char *message = "Now Connected To Debug Server";  
     
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
    if (listen(master_socket, 3) < 0)  
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
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
              
           
            //send new connection greeting message 
            if( send(new_socket, message, strlen(message), 0) != strlen(message) )  
            {  
                perror("send");  
            }  
                 
            puts("Welcome message sent successfully");  
                 
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
                    //set the string terminating NULL byte on the end 
                    //of the data read

                    //here add recording information about incoming message that is not a new connection
                    //so what type: ancmt send/ancmt receive/
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);
                    char *temp = inet_ntoa(address.sin_addr);
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
                    buffer[valread] = '\0';  
                    send(sd , buffer , strlen(buffer) , 0 );  
                }  
            }  
        }  
    }
    free(root);
    */
         
    return 0;  
}