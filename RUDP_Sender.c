#include "stdio.h"
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 5060

// A struct that represents RUDP Socket
typedef struct _rudp_socket {
    int socket_fd; // UDP socket file descriptor
    bool isServer; // True if the RUDP socket acts like a server, false for client.
    bool isConnected; // True if there is an active connection, false otherwise.
    struct sockaddr_in dest_addr; // Destination address. Client fills it when it connects via rudp_connect(), server fills it when it accepts a connection via rudp_accept().
} RUDP_Socket;

/*RUDP_Socket* rudp_socket(bool isServer, unsigned short int listen_port); 
int rudp_connect(RUDP_Socket *sockfd, const char *dest_ip, unsigned short int dest_port);
int rudp_send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size);
int rudp_close(RUDP_Socket *sockfd); */

int main()
{
    // Create socket
	RUDP_Socket* rudp_socket(bool isServer, unsigned short int listen_port); 

	// Setup the server address structure.
	// Port and IP should be filled in network byte order
    struct sockaddr_in RUDPreceiverAddress;
	memset(&RUDPreceiverAddress, 0, sizeof(RUDPreceiverAddress));
	RUDPreceiverAddress.sin_family = AF_INET;
	RUDPreceiverAddress.sin_port = htons(SERVER_PORT);
	int rudp_connect = rudp_connect(RUDP_Socket *sockfd, const char *dest_ip, unsigned short int dest_port);
	if (rudp_connect == 0){
		printf("rudp_connect failed");
		return -1;
	}
    else if(rudp_connect == 1){
        printf("rudp_connect success!");
    }

	//send the message
	int rudp_send = rudp_send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size);
    if(!isConnected){
        perror("The socket disconnected");
        return -1;
    }
    else if(udp_connect == 0){
        printf("The packet disconnected");
        return 0;
    }
    else{
        return rudp_send;
    }

	struct sockaddr_in fromAddress;
	//Change type variable from int to socklen_t: int fromAddressSize = sizeof(fromAddress);
	socklen_t fromAddressSize = sizeof(fromAddress);

	memset((char *)&fromAddress, 0, sizeof(fromAddress));

	// try to receive some data, this is a blocking call
	if (recvfrom(s, bufferReply, sizeof(bufferReply) -1, 0, (struct sockaddr *) &fromAddress, &fromAddressSize) == -1)
	{
		printf("recvfrom() failed");
		return -1;
	}
    int rudp_disconnect(RUDP_Socket *sockfd);
    if(rudp_disconnect == 1){
        printf("Disconnects from an actively connected socket success!");
    }
    else if(rudp_disconnect == 0){
        printf("The socket is already disconnected");
    }

	udp_close(rudp_socket);

    return 0;
}
