#include "stdio.h"
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>

#include "RUDP_API.h"

/*
* @brief A checksum function that returns 16 bit checksum for data.
* @param data The data to do the checksum for.
* @param bytes The length of the data in bytes.
* @return The checksum itself as 16 bit unsigned number.
* @note This function is taken from RFC1071, can be found here:
* @note https://tools.ietf.org/html/rfc1071
* @note It is the simplest way to calculate a checksum and is not very strong.
* However, it is good enough for this assignment.
* @note You are free to use any other checksum function as well.
* You can also use this function as such without any change.
*/
unsigned short int calculate_checksum(void *data, unsigned int bytes) {
    unsigned short int *data_pointer = (unsigned short int *)data;
    unsigned int total_sum = 0;
    // Main summing loop
    while (bytes > 1) {
        total_sum += *data_pointer++;
        bytes -= 2;
    }
    // Add left-over byte, if any
    if (bytes > 0){
        total_sum += *((unsigned char *)data_pointer);
    }
    // Fold 32-bit sum to 16 bits
    while (total_sum >> 16){
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
    }
    return (~((unsigned short int)total_sum));
}

RUDP_Socket* rudp_socket(bool isServer, unsigned short int listen_port){
    RUDP_Socket *sockfd = (RUDP_Socket *)malloc(sizeof(RUDP_Socket));
    if (sockfd == NULL) {
        perror("Failed to allocate memory for RUDP socket");
        return NULL;
    }

    sockfd->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd->socket_fd == -1) {
        perror("Failed to create socket");
        free(sockfd);
        return NULL;
    }

    sockfd->isServer = isServer;
    sockfd->isConnected = false;

    // Server-specific setup
    if (isServer) {
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(listen_port);

        if (bind(sockfd->socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Failed to bind socket");
            close(sockfd->socket_fd);
            free(sockfd);
            return NULL;
        }
    }

    return sockfd;
}

int rudp_connect(RUDP_Socket *sockfd, const char *dest_ip, unsigned short int dest_port){
    if (sockfd == NULL) { // There is no open socket
        fprintf(stderr, "Invalid RUDP socket\n");
        return 0;
    }

    if (sockfd->isServer) { // The socket is connected/set to server
        fprintf(stderr, "Cannot connect from server-side socket\n");
        return 0;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET; // IPv4
    dest_addr.sin_port = htons(dest_port);
    // inet_pton() converts IP addresses from their textual form to their binary form
    if (inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr) <= 0) { // The socket is already connect
        perror("Invalid address");
        return 0;
    }

    sockfd->dest_addr = dest_addr;
    sockfd->isConnected = true;

    return 1;
}

int rudp_accept(RUDP_Socket *sockfd){
    if (sockfd == NULL) { // There is no open socket
        fprintf(stderr, "Invalid RUDP socket\n");
        return 0;
    }

    if (!sockfd->isServer) { // The socket is connected/set to client
        fprintf(stderr, "Cannot accept on client-side socket\n");
        return 0;
    }

    return 1; // rudp_accept() successeded
}

int rudp_recv(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size){
    if (sockfd == NULL) {
        fprintf(stderr, "Invalid RUDP socket\n");
        return -1;
    }

    if (!sockfd->isConnected) {
        fprintf(stderr, "Socket is not connected\n");
        return 0;
    }

    ssize_t recv_len = recvfrom(sockfd->socket_fd, buffer, buffer_size, 0, NULL, NULL);
    if (recv_len == -1) {
        perror("recvfrom() failed \n");
        return -1;
    }

    unsigned int rudp_recv_checksum = calculate_checksum(sockfd, recv_len);
    if(rudp_recv_checksum == buffer_size){
        printf("The data received intactly. \n");
    }
    else{
        return -1;
    }

    return recv_len;
}
// -------------------- I need to continue from here------------
int rudp_send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size){
    if (sockfd == NULL) {
        fprintf(stderr, "Invalid RUDP socket\n");
        return -1;
    }

    if (!sockfd->isConnected) {
        fprintf(stderr, "Socket is not connected\n");
        return 0;
    }

    ssize_t sent_len = sendto(sockfd->socket_fd, buffer, buffer_size, 0, (struct sockaddr *)&(sockfd->dest_addr), sizeof(sockfd->dest_addr));
    if (sent_len == -1) {
        perror("sendto() failed");
        return -1;
    }

    return sent_len;
}

int rudp_disconnect(RUDP_Socket *sockfd){
    if(!sockfd->isConnected){
        printf("you are alredy discinnected");
        return 0;
    }
    sockfd->isConnected = false;
    return 1;
}

int rudp_close(RUDP_Socket *sockfd){
    close(sockfd->socket_fd);
    return 0;
}
