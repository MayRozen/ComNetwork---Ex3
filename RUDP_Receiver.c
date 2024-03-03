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

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 5060
#define BUFFER_SIZE 1024


int main()
{
	signal(SIGPIPE, SIG_IGN); // prevent crash on closing socket
    int rudp_socket = -1; // Open the listening (Receiver) socket
    char receive_buff[256], send_buff[256];
    struct sockaddr_in sender;
    int sender_size;
	struct timeval start_time, end_time;
    size_t total_file_size = 0; // Total file size
    double total_time_taken = 0; // Total time taken 

    if((rudp_socket = socket(AF_INET , SOCK_DGRAM , 0 )) == -1){
        printf("Could not create listening socket: ");
    }

	// setup Server address structure
	struct sockaddr_in RUDPreceiverAddress;
	memset((char *)&RUDPreceiverAddress, 0, sizeof(RUDPreceiverAddress));
	RUDPreceiverAddress.sin_family = AF_INET;
	RUDPreceiverAddress.sin_port = htons(SERVER_PORT);

	//Bind
	if (bind(rudp_socket, (struct sockaddr *)&RUDPreceiverAddress, sizeof(RUDPreceiverAddress)) == -1){
		printf("bind() failed");
		return -1;
	}
	printf("After bind(). Waiting for Sender");

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

		int recv_len = -1;
		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(rudp_socket, buffer, sizeof(buffer) -1, 0, (struct sockaddr *) &RUDPsenderAddress, &RUDPsenderAddressLen)) == -1){
			printf("recvfrom() failed");
			break;
		}

		gettimeofday(&start_time, NULL);

		int senderSocket = accept(rudp_socket, (struct sockaddr *)&RUDPsenderAddress, &RUDPsenderAddressLen);
    	if (senderSocket == -1){
            printf("listen failed with error code:");
            close(rudp_socket);
            return -1;
    	}

		ssize_t bytes_read = BUFFER_SIZE;
        size_t total_bytes_sent = 0;
        do {
            ssize_t random_data = recv(senderSocket, receive_buff, BUFFER_SIZE, 0);

            ssize_t bytes_sent = send(senderSocket, &random_data, bytes_read, 0);
            printf("bytes sent is: %zu\n", random_data);
            if (bytes_sent == -1) {
                perror("send() failed");
                close(senderSocket);
                return -1;
            }

            // Check if the received message is an exit message
            if (strncmp(receive_buff, "EXIT", 4) == 0) {
                printf("Received exit message from sender\n");
                break; // Exit the loop if the sender sends an exit message
            }
            total_bytes_sent += random_data;
            
        } while (bytes_read > 0);
        printf("the total bytes sent is: %zu\n", total_bytes_sent);
        if(total_bytes_sent < 2 * 1024 * 1024){ // Checking if the file is at least 2MB
            perror("The file's size is smaller than expected");
            close(senderSocket);
            close(rudp_socket);
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
        if (recvfrom(rudp_socket, (void*)receive_buff, sizeof(receive_buff), 0, (struct sockaddr*) &sender, &sender_size) < 0) {
            perror("failed to receive broadcast message");
            break;
            //return -1;
        }
        printf("%s\n", receive_buff);
      
    	printf("A new sender connection accepted\n");
    }
	

	rudp_close(rudp_socket);
	printf("RUDP_Receiver end");
	return 0;
}
