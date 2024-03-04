#include "RUDP_API.h"

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
// -------------------- I nedd to continue from here------------
int rudp_recv(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size){
    if (sockfd == NULL) {
        fprintf(stderr, "Invalid RUDP socket\n");
        return 0;
    }

    if (!sockfd->isConnected) {
        fprintf(stderr, "Socket is not connected\n");
        return -1;
    }

    ssize_t recv_len = recvfrom(sockfd->socket_fd, buffer, buffer_size, 0, NULL, NULL);
    if (recv_len == -1) {
        perror("recvfrom() failed");
        return -1;
    }

    return recv_len;
}

int rudp_send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size){

}

int rudp_disconnect(RUDP_Socket *sockfd){
    if(sockfd->isConnected == false){
        printf("you are alredy discinnected");
        return 0;
    }
    sockfd->isConnected = false;
    return 1;
}

int rudp_close(RUDP_Socket *sockfd){
    close(sockfd->socket_fd);
}
