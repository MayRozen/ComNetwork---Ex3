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


int main()
{
	signal(SIGPIPE, SIG_IGN); // prevent crash on closing socket
    int listeningSocket = -1; // Open the listening (Receiver) socket
    char receive_buff[256], send_buff[256];
    struct sockaddr_in sender;
    int sender_size;

	// setup Server address structure
	struct sockaddr_in RUDPreceiverAddress;
	memset((char *)&RUDPreceiverAddress, 0, sizeof(RUDPreceiverAddress));
	RUDPreceiverAddress.sin_family = sockfd;
	RUDPreceiverAddress.sin_port = htons(SERVER_PORT);
	rudp_connect(RUDP_Socket *sockfd, const char *dest_ip, unsigned short int dest_port);

	//Bind
	if (bind(listeningSocket, (struct sockaddr *)&RUDPreceiverAddress, sizeof(RUDPreceiverAddress)) == -1){
		printf("bind() failed");
		return -1;
	}
	printf("After bind(). Waiting for Sender");

	// setup Client address structure
	struct sockaddr_in RUDPsenderAddress;
	socklen_t RUDPsenderAddressLen = sizeof(RUDPsenderAddress);

	memset((char *)&RUDPsenderAddress, 0, sizeof(RUDPsenderAddress));

	//keep listening for data
	while (1)
	{
		fflush(stdout);

		// zero client address 
		memset((char *)&RUDPsenderAddress, 0, sizeof(RUDPsenderAddress));
		RUDPsenderAddressLen = sizeof(RUDPsenderAddress);

		//clear the buffer by filling null, it might have previously received data
		memset(buffer, '\0', sizeof (buffer));

		int recv_len = -1;

		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(s, buffer, sizeof(buffer) -1, 0, (struct sockaddr *) &RUDPsenderAddress, &RUDPsenderAddressLen)) == -1)
		{
			printf("recvfrom() failed");
			break;
		}

		char clientIPAddrReadable[32] = { '\0' };
		inet_ntop(AF_INET, &RUDPsenderAddress.sin_addr, clientIPAddrReadable, sizeof(clientIPAddrReadable));

		//print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", clientIPAddrReadable, ntohs(RUDPsenderAddress.sin_port));
		printf("Data is: %s\n", buffer);

		//now reply to the Client
		if (sendto(s, message, messageLen, 0, (struct sockaddr*) &RUDPsenderAddress, RUDPsenderAddressLen) == -1)
		{
			printf("sendto() failed");
			break;
		}
	}

	rudp_close(rudp_socket);
	return 0;
}
