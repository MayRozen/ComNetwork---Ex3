#include <stdio.h>
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

#define BUFFER_SIZE 2*1024*1024

int main(int argc, char *argv[]){
    printf("start reciever\n");
    if (argc != 2) {//if the user didn't send all the arguments 
        fprintf(stderr, "Usage: %s <congestion_algorithm>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    unsigned short int port = atoi(argv[1]);
	signal(SIGPIPE, SIG_IGN); // prevent crash on closing socket
    RUDP_Socket *sockfd = rudp_socket(true,port);
    char* receive_buff = NULL;
    struct sockaddr_in sender;
    socklen_t sender_size;
	struct timeval start_time, end_time;
    size_t total_file_size = 0; // Total file size
    double total_time_taken = 0; // Total time taken 
    if(sockfd->socket_fd == -1){
        printf("Could not create listening socket\n");
    }
    
	// setup Server address structure
	struct sockaddr_in RUDPreceiverAddress;
	memset((char *)&RUDPreceiverAddress, 0, sizeof(RUDPreceiverAddress));
	RUDPreceiverAddress.sin_family = AF_INET;
	RUDPreceiverAddress.sin_port = htons(port);
      
    // Accept and incoming connection
    printf("Waiting for incoming RUDP-connections...\n");

	// setup Sender address structure
	struct sockaddr_in RUDPsenderAddress;

	memset((char *)&RUDPsenderAddress, 0, sizeof(RUDPsenderAddress));
	//keep listening for data
	while (1){
		// zero Sender address 
		memset((char *)&RUDPsenderAddress, 0, sizeof(RUDPsenderAddress));
		int senderSocket = rudp_accept(sockfd);
    	if (senderSocket <= 0){
            printf("listen failed with error code\n");
            rudp_close(sockfd);
            return -1;
    	}
        
		int bytes_read = 0;
        size_t total_bytes_sent = 0;
        int random_data;
        //char* header_checksum = 0;
        //int header_length = 0;
        gettimeofday(&start_time, NULL);
        do {
            random_data = rudp_recv(sockfd, receive_buff, bytes_read);
            // int bytes_sent = rudp_Send(sockfd, "ACK", sizeof("ACK"));
            // if (bytes_sent == -1) {
            //     perror("send() failed\n");
            //     rudp_close(sockfd);
            //     return -1;
            // }

            // Check if the received message is an exit message
            if (random_data == 0) {
                printf("An EXIT massage has been received\n");
                rudp_disconnect(sockfd);
                break; // Exit the loop if the sender sends an exit message
            }
            else if(random_data < 0){
                 printf("receive failed\n");
                rudp_disconnect(sockfd);
                return -1;
            }
            total_bytes_sent += random_data;
            
        } while (bytes_read > 0);
        // int sender_header = rudp_recv(sockfd, header_checksum, header_length);
        // if (sender_header == -1) {
        //         perror("send() failed\n");
        //         rudp_close(sockfd);
        //         return -1;
        //     }
        // int checksum = calculate_checksum(receive_buff,total_bytes_sent);
        // if(checksum != (int)*header_checksum){
        //     printf("The data received isn't intactly\n");
        //     close(senderSocket);
        //     rudp_close(sockfd);
        //     return -1;
        // }
        // if(total_bytes_sent != header_length){
        //     printf("The data hasn't received entirety\n");
        //     close(senderSocket);
        //     rudp_close(sockfd);
        //     return -1;
        // }
        //rudp_Send(sockfd,"ACK",sizeof("ACK"));
        printf("the total bytes sent is: %zu\n", total_bytes_sent);
        if(total_bytes_sent < 2 * 1024 * 1024){ // Checking if the file is at least 2MB
            printf("The file's size is smaller than expected\n");
            close(senderSocket);
            rudp_close(sockfd);
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
        memset(receive_buff, 0, sizeof(&receive_buff));
        sender_size = sizeof(struct sockaddr_in);
        if (recvfrom(sockfd->socket_fd, receive_buff, sizeof(receive_buff), 0,(struct sockaddr *)&sender, &sender_size) < 0) {
            perror("failed to receive broadcast message\n");
            break;
        }
        printf("%s\n", receive_buff);
      
    	printf("A new sender connection accepted\n");
    }
	
	rudp_close(sockfd);
	printf("RUDP_Receiver end\n");
	return 0;
}
