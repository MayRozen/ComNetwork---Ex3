#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <sys/time.h> 

#define PORT 5060  // The port that the Receiver listens
#define BUFFER_SIZE 1024
  
int main()
{
    signal(SIGPIPE, SIG_IGN); // prevent crash on closing socket
    int listeningSocket = -1; // Open the listening (Receiver) socket
    char receive_buff[256], send_buff[256];
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
    receiverAddress.sin_port = htons(PORT);  // network order
      
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
    int senderSocket = accept(listeningSocket, (struct sockaddr *)&senderAddress, &senderAddressLen);
    if(senderSocket==-1){
        printf("accept() failed with error code :");
        close(listeningSocket);
        return -1; //close the socket
    }
    

    while (1)//need to check when to close the server socket
    {
    	memset(&senderAddress, 0, sizeof(senderAddress));
        senderAddressLen = sizeof(senderAddress);

        gettimeofday(&start_time, NULL);

        // int senderSocket = accept(listeningSocket, (struct sockaddr *)&senderAddress, &senderAddressLen);
    	// if (senderSocket == -1){
        //     printf("listen failed with error code:");
        //     close(listeningSocket);
        //     return -1;
    	// }

        ssize_t bytes_read;
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
        if (recvfrom(listeningSocket, (void*)receive_buff, sizeof(receive_buff), 0, (struct sockaddr*) &sender, &sender_size) < 0) {
            perror("failed to receive broadcast message");
            return -1;
        }
        printf("%s\n", receive_buff);
      
    	printf("A new sender connection accepted\n");
    }
    
    // close(senderSocket);
    close(listeningSocket);
    printf("Receiver end");

    return 0;
}