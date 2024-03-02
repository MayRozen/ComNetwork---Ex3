#include "RUDP_API.h"

#include "stdio.h"
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>


// A struct that represents RUDP Socket
typedef struct _rudp_socket{
    int socket_fd; // UDP socket file descriptor
    bool isServer; // True if the RUDP socket acts like a server, false for client.
    bool isConnected; // True if there is an active connection, false otherwise.
    struct sockaddr_in dest_addr; // Destination address. Client fills it when it connects via rudp_connect(), server fills it when it accepts a connection via rudp_accept().
} RUDP_Socket;

RUDP_Socket* rudp_socket(bool isServer, unsigned short int listen_port){
    
}

int rudp_connect(RUDP_Socket *sockfd, const char *dest_ip, unsigned short int dest_port){

}

int rudp_accept(RUDP_Socket *sockfd){

}

int rudp_recv(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size){

}

int rudp_send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size){

}

int rudp_disconnect(RUDP_Socket *sockfd){

}

int rudp_close(RUDP_Socket *sockfd){

}
