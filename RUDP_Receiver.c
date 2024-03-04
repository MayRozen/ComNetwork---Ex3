#include "stdio.h"
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h> 

#include "RUDP_API.h"
#include "RUDP_API.c"

#define SERVER_PORT 5060
#define BUFFER_SIZE 2*1024*1024


int main()
{
    printf("start reciever\n");
	signal(SIGPIPE, SIG_IGN); // prevent crash on closing socket
    RUDP_Socket *sockfd = rudp_socket(true,SERVER_PORT);
    char receive_buff[BUFFER_SIZE], send_buff[BUFFER_SIZE];
    struct sockaddr_in sender;
    int sender_size;
	struct timeval start_time, end_time;
    size_t total_file_size = 0; // Total file size
    double total_time_taken = 0; // Total time taken 
    if(sockfd->socket_fd == -1){
        printf("Could not create listening socket: ");
    }

	// setup Server address structure
	struct sockaddr_in RUDPreceiverAddress;
	memset((char *)&RUDPreceiverAddress, 0, sizeof(RUDPreceiverAddress));
	RUDPreceiverAddress.sin_family = AF_INET;
	RUDPreceiverAddress.sin_port = htons(SERVER_PORT);

	//Bind
	// if (bind(sockfd->socket_fd, (struct sockaddr *)&RUDPreceiverAddress, sizeof(RUDPreceiverAddress)) == -1){
	// 	printf("bind() failed\n");
	// 	return -1;
	// }
	// printf("After bind(). Waiting for Sender\n");

    // Make the socket listening; actually mother of all client sockets.
    if (listen(sockfd->socket_fd, 1) == -1) // 500 is a Maximum size of queue connection requests
											// number of concurrent connections 
    {
	perror("listen() failed with error code: ");
        rudp_close(sockfd);
        return -1; // close the socket
    }
      
    // Accept and incoming connection
    printf("Waiting for incoming RUDP-connections...\n");

	// setup Sender address structure
	struct sockaddr_in RUDPsenderAddress;
	socklen_t RUDPsenderAddressLen = sizeof(RUDPsenderAddress);

	memset((char *)&RUDPsenderAddress, 0, sizeof(RUDPsenderAddress));

	//keep listening for data
	while (1)
	{
		fflush(stdout);
		char* buffer;
		// zero Sender address 
		memset((char *)&RUDPsenderAddress, 0, sizeof(RUDPsenderAddress));
		RUDPsenderAddressLen = sizeof(RUDPsenderAddress);

		//clear the buffer by filling null, it might have previously received data
		memset(buffer, '\0', sizeof (buffer));

		// int recv_len = -1;
		// //try to receive some data, this is a blocking call
		// if ((recv_len = recvfrom(sockfd->socket_fd, buffer, sizeof(buffer) -1, 0, (struct sockaddr *) &RUDPsenderAddress, &RUDPsenderAddressLen)) == -1){
		// 	printf("recvfrom() failed");
		// 	break;
		// }

		gettimeofday(&start_time, NULL);

		int senderSocket = rudp_accept(sockfd);
    	if (senderSocket == 0){
            printf("listen failed with error code:");
            close(sockfd->socket_fd);
            return -1;
    	}
        sockfd->isConnected = true;
		ssize_t bytes_read = BUFFER_SIZE;
        size_t total_bytes_sent = 0;
        do {
            ssize_t random_data = rudp_recv(sockfd, receive_buff, BUFFER_SIZE);

            ssize_t bytes_sent = rudp_send(sockfd, &random_data, bytes_read);
            printf("bytes sent is: %zu\n", random_data);
            if (bytes_sent == -1) {
                perror("send() failed\n");
                close(senderSocket);
                return -1;
            }

            // Check if the received message is an exit message
            if (strncmp(receive_buff, "EXIT", 4) == 0) {
                printf("Received exit message from sender\n");
                rudp_disconnect(sockfd);
                break; // Exit the loop if the sender sends an exit message
            }
            total_bytes_sent += random_data;
            
        } while (bytes_read > 0);
        printf("the total bytes sent is: %zu\n", total_bytes_sent);
        if(total_bytes_sent < 2 * 1024 * 1024){ // Checking if the file is at least 2MB
            perror("The file's size is smaller than expected");
            close(senderSocket);
            close(sockfd->socket_fd);
            return -1;
        }

        gettimeofday(&end_time, NULL);

        long seconds = end_time.tv_sec - start_time.tv_sec;
        long micros = end_time.tv_usec - start_time.tv_usec;
        double milliseconds = (seconds * 1000) + (double)micros / 1000;
        printf("The time is: %.2f\n", milliseconds);
    
        double seconds_taken = seconds + (double)micros / 1000000; // Calculate the time taken in seconds
        double bandwidth = total_bytes_sent / seconds_taken; // Calculate the average bandwidth hadar change
        printf("Average bandwidth: %.2f bytes/second\n", bandwidth);

        size_t file_size = strlen(receive_buff); // Calculate the size of the file received
        total_file_size += file_size; // Accumulate total file size and total time taken
        total_time_taken += (seconds + (double)micros / 1000000);
        double total_average_bandwidth = total_file_size / total_time_taken; // Calculate the average bandwidth
        printf("Total Average Bandwidth: %.2f bytes/second\n", total_average_bandwidth);

        printf("Listening...\n");
        memset(receive_buff, 0, sizeof(receive_buff));
        sender_size = sizeof(sender);
        if (recvfrom(sockfd->socket_fd, (void*)receive_buff, sizeof(receive_buff), 0, (struct sockaddr*) &sender, &sender_size) < 0) {
            perror("failed to receive broadcast message");
            break;
            //return -1;
        }
        printf("%s\n", receive_buff);
      
    	printf("A new sender connection accepted\n");
    }
	

	rudp_close(sockfd);
	printf("RUDP_Receiver end\n");
	return 0;
}