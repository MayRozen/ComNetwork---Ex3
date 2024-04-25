#include <stdio.h>
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
#define BUFFER_SIZE 2*1024*1024



char *util_generate_random_data(unsigned int size) {
    char *buffer = NULL;
    // Argument check.
    if (size == 0)
        return NULL;
    buffer = (char *)calloc(size, sizeof(char));
    // Error checking.
    if (buffer == NULL)
        return NULL;
    // Randomize the seed of the random number generator.
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++)
        *(buffer + i) = ((unsigned int)rand() % 256);
    return buffer;
}

int main(int argc, char *argv[]){
    printf("start of the RUDP_Sender\n");
    if (argc != 3) {//if the user didn't send all the arguments 
        fprintf(stderr, "Usage: %s <congestion_algorithm>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Create socket
    
    unsigned short int port = atoi(argv[1]);
    const char *server_ip = argv[2];
	RUDP_Socket* rudpSocket = rudp_socket(false,port);
    unsigned int size2 = 2*1024*1024;
    char *random_data = util_generate_random_data(size2); //Our file  
    //Header sender_header;

    if(rudpSocket->socket_fd == -1){
        perror("failed to create socket\n"); //The socket uncreated
        return -1;
    }

	// Setup the server address structure.
	// Port and IP should be filled in network byte order
    
	int rudpConnect = rudp_connect(rudpSocket,server_ip,port);
	if (rudpConnect <= 0){
		printf("rudp_connect failed\n");
		return -1;
	}   
    printf("the sender is connected\n");        
    
	do{
        char tmpbuffer[BUFFER_SIZE];
        //send the message
        int byteSent = rudp_Send(rudpSocket,random_data,size2);
        printf("sent data to receiver\n");
        printf("the total byte sent is %d\n",byteSent);
        if(byteSent<=0){
            free(random_data);
            close(rudpSocket->socket_fd);
            return -1;
        }
        rudp_recv(rudpSocket, tmpbuffer, sizeof(tmpbuffer));
        printf("The massage is: %c\n", *tmpbuffer);
        // if (strncmp(tmpbuffer, "ACK", sizeof("ACK")) < 0){ //Here!!!!!!!!!!!!
        //      printf("Acknowledgment hasn't received, break the loop\n");
        //      free(random_data);
        //      close(rudpSocket->socket_fd);
        //      return -1;
        //  } 
        //rudp_recv(rudpSocket, tmpbuffer, sizeof(tmpbuffer));
        printf("Send the file again? y/n\n");
        char c;
        do {
            c = getchar();
        } while (c != 'y' && c != 'n' && c != '\n');  // Clear input buffer

        if (c == 'n') {
            printf("The socket will be closed\n");
            rudp_Send(rudpSocket,"EXIT", sizeof("EXIT"));
            break;
        }
    }while(size2 > 0); 

	struct sockaddr_in fromAddress;

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
    printf("The RUDP_Sender was successfully closed!\n");
    return 0;
}

