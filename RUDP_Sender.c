#include "stdio.h"
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#include "RUDP_API.h"
#include "RUDP_API.c"

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 5060

char *util_generate_random_data(unsigned int size){
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

int main()
{
    printf("start of the RUDP_Sender\n");
    // Create socket
	RUDP_Socket* rudpSocket = rudp_socket(false,SERVER_PORT);
    unsigned int size2 = 2*1024*1024;
    char* random_data = util_generate_random_data(size2); //Our file  

    if(rudpSocket->socket_fd == -1){
        perror("failed to create socket\n"); //The socket uncreated
        return -1;
    }

	// Setup the server address structure.
	// Port and IP should be filled in network byte order
    struct sockaddr_in RUDPreceiverAddress;
	memset(&RUDPreceiverAddress, 0, sizeof(RUDPreceiverAddress));
	RUDPreceiverAddress.sin_family = AF_INET;
	RUDPreceiverAddress.sin_port = htons(SERVER_PORT);

	int rudpConnect = rudp_connect(rudpSocket,SERVER_IP_ADDRESS,SERVER_PORT);
	if (rudpConnect <= 0){
		printf("rudp_connect failed\n");
		return -1;
	}           
    else if(rudpConnect == 1){
        printf("rudp_connect success!\n");
    }

	while(1){
        //send the message
        int rudpSend = rudp_send(rudpSocket,random_data,size2);
        // if(!rudpSocket->isConnected){
        //     perror("The socket disconnected\n");
        //     return -1;
        // }
        // else if(rudpConnect == 0){
        //     printf("The packet disconnected\n");
        //     return 0;
        // }
        // else{
        //     return rudpSend;
        // }
        if(rudpSend<=0){
            free(random_data);
            close(rudpSocket->socket_fd);
            return -1;
        }
        printf("Send the file again? y/n\n");
        char c = getchar();
        if(c == 'n'){
            rudp_send(rudpSocket,"EXIT",4);
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

    int isDisconnected = rudp_disconnect(rudpSocket);
    if(isDisconnected == 1){
        printf("Disconnects from an actively connected socket success!\n");
    }
    else if(isDisconnected == 0){
        printf("The socket is already disconnected\n");
    }

    free(random_data);
	rudp_close(rudpSocket);
    return 0;
}