#include<stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define PORT 5060  //The port that the server listens
  
int main()
{
    signal(SIGPIPE, SIG_IGN); //prevent crash on closing socket

    int listeningSocket = -1; // Open the listening (server) socket
	 
    if((listeningSocket = socket(AF_INET , SOCK_STREAM , 0 )) == -1){
        printf("Could not create listening socket: %d" );
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    struct sockaddr_in receiverAddress;
    memset(&receiverAddress, 0, sizeof(receiverAddress));

    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.s_addr = INADDR_ANY;
    receiverAddress.sin_port = htons(PORT);  //network order
      
    // Bind the socket to the port with any IP at this port
    if (bind(listeningSocket, (struct sockaddr *)&receiverAddress , sizeof(receiverAddress)) == -1){
        printf("Bind failed with error code : %d");
        return -1; //close the socket
    }

    // Reuse the address if the server socket on was closed
	// and remains for 45 seconds in TIME-WAIT state till the final removal.
    int enableReuse = 1;
    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0){
        perror("setsockopt");
    }
      
    printf("Bind() success\n");
  
    // Make the socket listening; actually mother of all client sockets.
    if (listen(listeningSocket, 500) == -1) //500 is a Maximum size of queue connection requests
											//number of concurrent connections 
    {
	printf("listen() failed with error code : %d");
        return -1; //close the socket
    }
      
    //Accept and incoming connection
    printf("Waiting for incoming TCP-connections...\n");
    int accept(listeningSocket, receiverAddress, sizeof(receiverAddress));
      
    struct sockaddr_in clientAddress; 
    socklen_t clientAddressLen = sizeof(clientAddress);

    while (1)
    {
    	memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddressLen = sizeof(clientAddress);
        int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
    	if (clientSocket == -1){
           printf("listen failed with error code : %d");
	   // TODO: close the sockets
           return -1;
    	}
      
    	printf("A new client connection accepted\n");
  
    	//Reply to client
    	char message[] = "Welcome to our TCP-server\n";
        int messageLen = strlen(message) + 1;

    	int bytesSent = send(clientSocket, message, messageLen, 0);
		if(-1 == bytesSent){
			printf("send() failed with error code : %d" );
		}
		else if(0 == bytesSent){
		   printf("peer has closed the TCP connection prior to send().\n");
		}
		else if(messageLen > bytesSent){
		   printf("sent only %d bytes from the required %d.\n", messageLen, bytesSent);
		}
		else {
		   printf("message was successfully sent .\n");
		}

    }
  
    // TODO: All open clientSocket descriptors should be kept
    // in some container and closed as well.
    close(listeningSocket);

      
    return 0;
}