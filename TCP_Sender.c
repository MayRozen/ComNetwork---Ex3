#include <stdio.h>
#include <stdlib.h> 
#include <errno.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 5060
#define IPADDRESS "192.168.1.1"
#define SENDER_FILE "SenderFile.dat" // The file
#define BUFFER_SIZE 1024

/*
* @brief
A random data generator function based on srand() and rand().
* @param size
The size of the data to generate (up to 2^32 bytes).
* @return
A pointer to the buffer.
*/
char *util_generate_random_data(unsigned int size) {
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

int main(){
    printf("start of the sender\n");
    int sock = socket(AF_INET, SOCK_STREAM, 0); //IPv4, TCP, defulte
    unsigned int size2 = 2*1024*1024;
    char* random_data = util_generate_random_data(size2); //Our file hadar change!!!!!!!!!!!!!!!!!

    if(sock == -1){
        perror("failed to create socket\n"); //The socket uncreated
        return -1;
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    
    struct sockaddr_in receiverAddress; //IPv4 only!
    memset(&receiverAddress, 0, sizeof(receiverAddress)); //Reset before every use
    receiverAddress.sin_family = AF_INET; //Address family, AF_INET unsigned.
    receiverAddress.sin_port = htons(PORT); //PORT number - bigqlittle endian

    struct in_addr sin_addr; //Internet address
    char sin_zero[2]; //2 MB

	int rval = inet_pton(AF_INET, IPADDRESS, &receiverAddress.sin_addr); //Casting to binary - 0=secceeded, 1=fail.
    if (rval <= 0){
		printf("inet_pton() failed\n");
		return -1;
	}
    
     // Make a connection to the Receiver with socket SendingSocket.

    if (connect(sock, (struct sockaddr *) &receiverAddress, sizeof(receiverAddress)) == -1) //Connection with Receiver - if the connection unsucceed, it will return -1
    {
	   printf("connect() failed with error code :\n" );
    }

    printf("connected to Receiver\n");

    char ip4[INET_ADDRSTRLEN] = {0}; // Sends the file to Receiver
    inet_ntop(AF_INET, &(receiverAddress.sin_addr), ip4, INET_ADDRSTRLEN);
    printf("The IPv4 address is: %s\n", ip4);
    
    int bytesSent = send(sock, ip4, INET_ADDRSTRLEN + 1, 0);

    do {
        ssize_t bytes_sent = send(sock, random_data, size2, 0);  // Use size2 instead of sizeof(random_data)
        if (bytes_sent == -1) {
            perror("send() failed\n");
            free(random_data);
            close(sock);
            return -1;
        }

        printf("Do you want to send the file again? y/n\n");
        char c;
            do {
                c = getchar();
            } while (c != 'y' && c != 'n' && c != '\n');  // Clear input buffer

            if (c == 'n') {
                break;
            }
    } while (size2 > 0);

    if(-1 == bytesSent){
	    printf("send() failed with error code :\n" );
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
    free(random_data);
    close(sock);

    return 0; //Exit
}