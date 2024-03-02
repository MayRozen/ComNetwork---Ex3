#include "stdio.h"
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <RUDP_API.h>

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 5060

char *util_generate_random_data(unsigned int size) {
    char *buffer;
    // Argument check.
    if (size == 0){
        return NULL;
    }
    buffer = (char *)calloc(size, sizeof(char));
    // Error checking.
    if (buffer == NULL){
        return NULL;
    }
    // Randomize the seed of the random number generator.
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++){
        *(buffer + i) = ((unsigned int)rand() % 256);
    }
    return buffer;
}


/*RUDP_Socket* rudp_socket(bool isServer, unsigned short int listen_port); 
int rudp_connect(RUDP_Socket *sockfd, const char *dest_ip, unsigned short int dest_port);
int rudp_send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size);
int rudp_close(RUDP_Socket *sockfd); */

int main()
{
    printf("start of the RUDP_Sender\n");
    // Create socket
	int rudp_socket = RUDP_Socket* rudp_socket(bool isServer, unsigned short int listen_port);
    unsigned int size2 = 2*1024*1024;
    char* random_data = util_generate_random_data(size2); //Our file  

	// Setup the server address structure.
	// Port and IP should be filled in network byte order
    struct sockaddr_in RUDPreceiverAddress;
	memset(&RUDPreceiverAddress, 0, sizeof(RUDPreceiverAddress));
	RUDPreceiverAddress.sin_family = sockfd;
	RUDPreceiverAddress.sin_port = htons(SERVER_PORT);
	int rudp_connect = rudp_connect(RUDP_Socket *sockfd, const char *dest_ip, unsigned short int dest_port);
	if (rudp_connect == 0){
		printf("rudp_connect failed");
		return -1;
	}
    else if(rudp_connect == 1){
        printf("rudp_connect success!");
    }

	while(1){
        //send the message
        int rudp_send = rudp_send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size);
        if(!isConnected){
            perror("The socket disconnected");
            return -1;
        }
        else if(rudp_connect == 0){
            printf("The packet disconnected");
            return 0;
        }
        else{
            return rudp_send;
        }

        printf("Send the file again? y/n\n");
        char c = getchar();
        if(c == 'n'){
            break;
        }
        else{
            continue;
        }
    }

	struct sockaddr_in fromAddress;
	//Change type variable from int to socklen_t: int fromAddressSize = sizeof(fromAddress);
	socklen_t fromAddressSize = sizeof(fromAddress);

	memset((char *)&fromAddress, 0, sizeof(fromAddress));

	// try to receive some data, this is a blocking call
	if (recvfrom(s, bufferReply, sizeof(bufferReply) -1, 0, (struct sockaddr *) &fromAddress, &fromAddressSize) == -1){
		printf("recvfrom() failed");
        free(random_data);
        rudp_close(rudp_socket);
        return -1;
	}
    int rudp_disconnect(RUDP_Socket *sockfd);
    if(rudp_disconnect == 1){
        printf("Disconnects from an actively connected socket success!");
    }
    else if(rudp_disconnect == 0){
        printf("The socket is already disconnected");
    }

	rudp_close(rudp_socket);
    return 0;
}
