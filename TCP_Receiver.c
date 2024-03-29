#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <netinet/tcp.h>

#include <sys/time.h> 

#define BUFFER_SIZE 1024

  
int main(int argc,char *argv[])
{
    printf("starting Reciever...\n");
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <congestion_algorithm>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char *congestion_algo = argv[2];
    const int port = atoi(argv[1]);
    signal(SIGPIPE, SIG_IGN); // prevent crash on closing socket
    int listeningSocket = -1; // Open the listening (Receiver) socket
    char receive_buff[BUFFER_SIZE], send_buff[BUFFER_SIZE];
    struct sockaddr_in sender;
    int sender_size;
    struct timeval start_time, end_time;
    size_t total_file_size = 0; // Total file size
    double total_time_taken = 0; // Total time taken 
	 
    if((listeningSocket = socket(AF_INET , SOCK_STREAM , 0 )) == -1){
        printf("Could not create listening socket: ");
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    struct sockaddr_in receiverAddress;
    memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.s_addr = INADDR_ANY;
    receiverAddress.sin_port = htons(port);  // network order
      
    // Bind the socket to the port with any IP at this port
    if (bind(listeningSocket, (struct sockaddr *)&receiverAddress , sizeof(receiverAddress)) == -1){
        printf("Bind failed with error code: ");
        close(listeningSocket);
        return -1; // close the socket
    }

    // Reuse the address if the Receiver socket on was closed
	// and remains for 45 seconds in TIME-WAIT state till the final removal.
    int enableReuse = 1;
    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0){
        perror("setsockopt");
    }
      
    printf("Bind() success\n");
  
    // Make the socket listening; actually mother of all client sockets.
    if (listen(listeningSocket, 500) == -1) // 500 is a Maximum size of queue connection requests
											// number of concurrent connections 
    {
	printf("listen() failed with error code: ");
        close(listeningSocket);
        return -1; // close the socket
    }
      
    // Accept and incoming connection
    printf("Waiting for incoming TCP-connections...\n");
    
    struct sockaddr_in senderAddress; 
    socklen_t senderAddressLen = sizeof(senderAddress);

    socklen_t receiverAddressSize = sizeof(receiverAddress);
    
    memset(&senderAddress, 0, sizeof(senderAddress));
    senderAddressLen = sizeof(senderAddress);

    gettimeofday(&start_time, NULL);

    int senderSocket = accept(listeningSocket, (struct sockaddr *)&senderAddress, &senderAddressLen);
    if (senderSocket == -1){
        printf("listen failed with error code:");
        close(listeningSocket);
        return -1;
    	}
    if (setsockopt(senderSocket, IPPROTO_TCP, TCP_CONGESTION, congestion_algo, strlen(congestion_algo)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    printf("Sender connected, beginning to recieve file...\n");
    ssize_t bytes_read = BUFFER_SIZE;
    size_t total_bytes_sent = 0;
    ssize_t random_data;
    do {
        random_data = recv(senderSocket, receive_buff, bytes_read, 0);

        //ssize_t bytes_sent = send(senderSocket, &random_data, bytes_read, 0);
        ssize_t bytes_sent = send(senderSocket, receive_buff, random_data/2, 0);
        if (bytes_sent == 0) {
            perror("send() failed");
            close(senderSocket);
            return -1;
        }
        // printf("File trasfer completed.\n");
        // printf("Waiting for Sender response...\n");

        // Check if the received message is an exit message
        if (strncmp(receive_buff, "EXIT", 4) == 0) {
            printf("Received exit message from sender\n");
            break; // stop the listening if the sender sends an exit message
        }
        total_bytes_sent += random_data;
            
    } while (bytes_read > 0);

    if(total_bytes_sent < 2 * 1024 * 1024){ // Checking if the file is at least 2MB
        perror("The file's size is smaller than expected");
        close(senderSocket);
        close(listeningSocket);
        return -1;
    }

    gettimeofday(&end_time, NULL);
    printf("           *Statistics*\n");
    long seconds = end_time.tv_sec - start_time.tv_sec;
    long micros = end_time.tv_usec - start_time.tv_usec;
    double milliseconds = (seconds * 1000) + (double)micros / 1000;
    printf("The time is: %.2f\n", milliseconds);
    
    double seconds_taken = seconds + (double)micros / 1000000; // Calculate the time taken in seconds
    double bandwidth = total_bytes_sent / seconds_taken; // Calculate the average bandwidth hadar change
    printf("Average bandwidth: %.2f bytes/second\n", bandwidth);

    size_t file_size = random_data; // Calculate the size of the file received
    total_file_size += file_size; // Accumulate total file size and total time taken
    total_time_taken += (seconds + (double)micros / 1000000);
    double total_average_bandwidth = total_file_size / total_time_taken; // Calculate the average bandwidth
    printf("Total Average Bandwidth: %.2f bytes/second\n", total_average_bandwidth);

    close(senderSocket);
    close(listeningSocket);
    printf("Reciever end\n");
    return 0;
}