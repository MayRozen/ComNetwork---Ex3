#include <stdio.h>
#include <stdlib.h> 
#include <errno.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>

// file
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#define PORT 5060
#define SENDER_FILE "SenderFile.dat" // The file to sen

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0); //IPv4, TCP, defulte

    if(sock == -1)
    {
        perror("failed to create socket"); //The socket uncreated
        return -1;
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    
    struct sockaddr_in receiverAddress; //IPv4 only!
    memset(&receiverAddress, 0, sizeof(receiverAddress)); //Reset before every use
    receiverAddress.sin_family = AF_INET; //Address family, AF_INET unsigned.
    receiverAddress.sin_port = htons(PORT); //PORT number - bigqlittle endian

    struct in_addr sin_addr; //Internet address
    unsigned char sin_zero[2]; //2 MB

	int rval = inet_pton(AF_INET, (const char*)PORT, &receiverAddress.sin_addr); //Casting to binary - 0=secceeded, 1=fail.
	if (rval <= 0){
		printf("inet_pton() failed");
		return -1;
	}

     // Make a connection to the server with socket SendingSocket.

    if (connect(sock, (struct sockaddr *) &receiverAddress, sizeof(receiverAddress)) == -1) //Connection with server - if the connection unsucceed, it will return -1
    {
	   printf("connect() failed with error code : %d" );
    }

    printf("connected to server\n");

    // Sends some data to server
    char ip4[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &(receiverAddress.sin_addr), ip4, INET_ADDRSTRLEN);
    printf("The IPv4 address is: %s\n", ip4);

    int bytesSent = send(sock, ip4, INET_ADDRSTRLEN+1, 0); //File

    if(-1 == bytesSent){
	printf("send() failed with error code : %d" );
    }
    else if(0 == bytesSent){
	printf("peer has closed the TCP connection prior to send().\n");
    }
    else if(INET_ADDRSTRLEN+1 > bytesSent){
	printf("sent only %d bytes from the required %d.\n", INET_ADDRSTRLEN+1, bytesSent);
    }
    else {
	printf("message was successfully sent .\n");
    }

	sleep(3);

    // All open clientSocket descriptors should be kept
    // in some container and closed as well.
    close(sock);

    return 0; //Exit
}
