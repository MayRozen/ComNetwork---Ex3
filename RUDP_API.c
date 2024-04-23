#include <stdio.h>
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#include "RUDP_API.h"
#define MAX_UDP 1472
#define BUFFER_SIZE 2*1024*1024
#define MAX_UDP_PAYLOAD_SIZE 65507 // Maximum UDP payload size
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

int ACKtimeOut(int socket, int seqNumber, clock_t start, int timeout){
    Packet *packet = malloc(sizeof(Packet));
    while ((double)(clock() - start) / CLOCKS_PER_SEC < timeout) {
        int recvLen = recvfrom(socket, packet, sizeof(Packet) - 1, 0, NULL, 0);
        if (recvLen == -1) {
        free(packet);
        return 0;
        }
        if (packet->header.seqNum == seqNumber && packet->header.flag.ACK == 1) {
        free(packet);
        return 1;
        }
    }
  free(packet);
  return 0;
} 

int timeoutSeting(int socket, int time) {
  // set timeout for the socket
  struct timeval timeout;
  timeout.tv_sec = time;
  timeout.tv_usec = 0;
  if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    perror("setting the timeout for socket didn't succeded");
    return -1;
  }
  return 1;
}

RUDP_Socket* rudp_socket(bool isServer, unsigned short int listen_port){
    RUDP_Socket *sockfd = (RUDP_Socket *)malloc(sizeof(RUDP_Socket));
    if (sockfd == NULL) {
        perror("Failed to allocate memory for RUDP socket");
        return NULL;
    }

    sockfd->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd->socket_fd == -1) {
        perror("Failed to create socket");
        free(sockfd);
        return NULL;
    }

    sockfd->isServer = isServer;
    sockfd->isConnected = false;

    // Server-specific setup
    if (sockfd->isServer) {
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

            // Reuse the address if the Receiver socket on was closed
        // and remains for 45 seconds in TIME-WAIT state till the final removal.
        int enableReuse = 1;
        if (setsockopt(sockfd->socket_fd, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0){
            perror("setsockopt");
        }
        
        printf("Bind() success\n");
    }
    printf("socket build succsesfuly\n");
    return sockfd;
}

int rudp_connect(RUDP_Socket *sockfd, const char *dest_ip, unsigned short int dest_port){
    if (timeoutSeting(sockfd->socket_fd, 1) == -1) {
        return -1;
    }
    if (sockfd == NULL) { // There is no open socket
        fprintf(stderr, "Invalid RUDP socket\n");
        return 0;
    }

    if (sockfd->isServer) { // The socket is connected/set to server
        fprintf(stderr, "Cannot connect from server-side socket\n");
        return 0;
    }
    int recvBytes;
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET; // IPv4
    dest_addr.sin_port = htons(dest_port);
    sockfd->dest_addr = dest_addr;

    // inet_pton() converts IP addresses from their textual form to their binary form
    if (inet_pton(AF_INET, dest_ip, &sockfd->dest_addr.sin_addr) <= 0) { // The socket is already connect
        perror("Invalid address");
        return 0;
    }
    //sockfd->dest_addr = dest_addr;
    if (connect(sockfd->socket_fd, (struct sockaddr *)&sockfd->dest_addr,sizeof(sockfd->dest_addr)) == -1) {
        perror("connect failed");
        return -1;
    }
    Packet *packet = malloc(sizeof(Packet));
    memset(packet, 0, sizeof(Packet));
    packet->header.flag.SYN = 1;//SYN
    int total_tries = 0;
    Packet *recv_packet = NULL;
    while (total_tries < 3) {
        if (sendto(sockfd->socket_fd, packet, sizeof(Packet), 0, (struct sockaddr*)&sockfd->dest_addr, sizeof(sockfd->dest_addr)) == -1) {
            perror("Error sending connection request");
            free(packet);
            return -1;
        }
        clock_t startingTime = clock();
        do {
            // receive SYN-ACK message
            recv_packet = malloc(sizeof(Packet));
            memset(recv_packet, 0, sizeof(Packet));
            recvBytes = recvfrom(sockfd->socket_fd, recv_packet, sizeof(Packet), 0, NULL, 0);
            if (recvBytes == -1) {//problom!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                perror("recvfrom failed");
                return -1;
            }
            if (recv_packet->header.flag.SYN == 1 && recv_packet->header.flag.ACK == 1) {
                printf("connected\n");
                free(packet);
                free(recv_packet);
                sockfd->isConnected = true;
                return 1;
            } 
            else {
                printf("received wrong packet when trying to connect");
            }
        } while ((double)(clock() - startingTime) / CLOCKS_PER_SEC < 1);
        total_tries++;
    }
    
    printf("Connection request failed\n");
    free(packet);
    free(recv_packet);
    return 0;
}

int rudp_accept(RUDP_Socket *sockfd){
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);
    memset((char *)&clientAddress, 0, sizeof(clientAddress));
    
    Packet *packet = malloc(sizeof(Packet));
    memset(packet, 0, sizeof(Packet));
    int recv_len = recvfrom(sockfd->socket_fd, packet, sizeof(Packet) - 1, 0,(struct sockaddr *)&clientAddress, &clientAddressLen);
    if (recv_len == -1) {
        perror("recvfrom() failed on receiving SYN");
        free(packet);
        return -1;
    }

    if (connect(sockfd->socket_fd, (struct sockaddr *)&clientAddress, clientAddressLen) == -1) {
        perror("connect() failed with error code");
        free(packet);
        return -1;
    }

    if (packet->header.flag.SYN == 1) {
        printf("received SYN request\n");
        Packet *packetReply = malloc(sizeof(Packet));
        memset(packet, 0, sizeof(Packet));
        packetReply->header.flag.SYN = 1;
        packetReply->header.flag.ACK = 1;
        if (sendto(sockfd->socket_fd, packetReply, sizeof(Packet), 0, NULL, 0) == -1) {
            printf("sendto() failed with error code  : %d", errno);
            free(packet);
            free(packetReply);
            return -1;
        }
        timeoutSeting(sockfd->socket_fd, 1 * 10);
        free(packet);
        free(packetReply);
        sockfd->isConnected = true;
        printf("rudp_accept() successeded\n");
        return 1;
    }
    return 0;
}

int rudp_recv(RUDP_Socket *sockfd, void **buffer, int *buffer_size){
    if (sockfd == NULL) {//if the socket isn't valid
        fprintf(stderr, "Invalid RUDP socket\n");
        return -1;
    }
    if (!sockfd->isConnected) {//if the sender is not connected
        printf("Receive failed. Socket is not connected\n");
        return -1;
    }
    printf("testing\n");
    int sqNum = 0;
    Packet *packet = malloc(sizeof(Packet));
    if(packet == NULL){
        printf("malloc failed\n");
        return -1;
    }
    memset(packet, 0, sizeof(Packet));

    int recv_len = recvfrom(sockfd->socket_fd, packet, sizeof(Packet) , 0, NULL, 0);
    if (recv_len == -1) {
        printf("recvfrom() failed : %d", errno);
        free(packet);
        return -1;
    }
    // check if the packet is corrupted, and send ack
    if (calculate_checksum(packet->data,packet->header.length) != packet->header.checksum) {
        printf("invalid data transform\n");
        free(packet);
        return -1;
    }
    if (rudp_Send(sockfd, "ACK", sizeof("ACK")) == -1){
        perror("send() failed\n");
        free(packet);
        return -1;
    }
    // if (packet->header.flag.SYN == 1) {  // connection request
    //     printf("received connection request\n");
    //     free(packet);
    //     return 0;
    // }
    if (packet->header.seqNum == sqNum) {
        if (packet->header.seqNum == 0 && packet->header.flag.DATA == 1) {
            timeoutSeting(sockfd->socket_fd, 1 * 10);//ten seconds
        }
        if (packet->header.flag.ACK == 1 && packet->header.flag.SYN == 1) {  // last packet
            *buffer = malloc(packet->header.length);  // Allocate memory for data
            memcpy(buffer, packet->data, packet->header.length);
            *buffer_size = packet->header.length;
            sqNum = 0;
            timeoutSeting(sockfd->socket_fd, 10000000);
            free(packet);
            return recv_len;
        }
        if (packet->header.flag.DATA == 1) {     // data packet
            *buffer = malloc(packet->header.length);  // Allocate memory for data
            memcpy(*buffer, packet->data, packet->header.length);
            *buffer_size = packet->header.length;
            free(packet);
            sqNum++;
            return recv_len;
        }
    } 
    else if (packet->header.flag.DATA == 1) {
        free(packet);
        return 0;
    }

    // ssize_t recv_len = recvfrom(sockfd->socket_fd, buffer, buffer_size, 0, NULL, NULL);
    // if (recv_len == -1) {
    //     printf("recvfrom() failed");
    //     return -1;
    // }

    if(sockfd->isServer){
        if(packet->header.flag.FIN == 1){
            printf("sender sent exit message\n");
            free(packet);
            rudp_close(sockfd);
            return 0; 
        }
    }
    printf("received %d byte\n",recv_len);
    free(packet);
    //free(buffer);
    return recv_len;
}

int rudp_Send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size){
    if (sockfd == NULL) {
        fprintf(stderr, "Invalid RUDP socket\n");
        free(buffer);
        close(sockfd->socket_fd);
        return 0;
    }

    if (!sockfd->isConnected){
        fprintf(stderr, "Socket is not connected\n");
        free(buffer);
        close(sockfd->socket_fd);
        return -1;
    }
    // calculate the number of packets needed to send the data
    int packets_num = buffer_size / MAX_SIZE;
    // calculate the size of the last packet
    int last_packet_size = buffer_size % MAX_SIZE;

    Packet *packet = malloc(sizeof(Packet));// need to free!!!!!!!!!!!!!!!!!!!!!!!!!!!
    if(packet == NULL){
        printf("malloc failed\n");
        return -1;
    }
    int sent_len = 0;
    int sent_total = 0;
    // send the packets
    for (int i = 0; i < packets_num; i++){
        memset(packet, 0, sizeof(Packet));
        packet->header.seqNum = i;     // set the sequence number
        packet->header.flag.DATA = 1;  // set the DATA flag
        // if we are at the last packet, we will set the FIN flag to 1
        if (i+1 == packets_num && last_packet_size == 0) {
            packet->header.flag.FIN = 1;
        }
        // put the data in the packet
        memcpy(packet->data, buffer + i * MAX_SIZE, MAX_SIZE);
        // set the length of the packet
        packet->header.length = MAX_SIZE;
        // calculate the checksum of the packet before the actual sending
        packet->header.checksum = calculate_checksum(packet->data,packet->header.length);

        // ssize_t sent_total = 0;
        // ssize_t remaining = buffer_size;
        // char *buffer_ptr = (char *)buffer;
        do {
        // ssize_t chunk_size = remaining > MAX_UDP_PAYLOAD_SIZE ? MAX_UDP_PAYLOAD_SIZE : remaining;
            sent_len = sendto(sockfd->socket_fd, packet, sizeof(Packet), 0, (struct sockaddr *)&(sockfd->dest_addr), sizeof(sockfd->dest_addr));
            if (sent_len == -1) {
                perror("sendto() failed\n");
                return -1;  // Handle the error appropriately
            }
            // sent_total += sent_len;
            // remaining -= sent_len;
            // buffer_ptr += sent_total;
        }while (ACKtimeOut(sockfd->socket_fd, i, clock(), 1) <= 0);
        sent_total += sent_len;
    }
    if (last_packet_size > 0) {
        memset(packet, 0, sizeof(Packet));
        // set the fields of the packet
        packet->header.seqNum = packets_num;
        packet->header.flag.DATA = 1;
        packet->header.flag.FIN = 1;
        memcpy(packet->data, buffer + packets_num * MAX_SIZE, last_packet_size);
        packet->header.length = last_packet_size;
        packet->header.checksum = calculate_checksum(packet->data,packet->header.length);
        do {  // send the packet and wait for ack
            int sendLastPacket = sendto(sockfd->socket_fd, packet, sizeof(Packet), 0, NULL, 0);
            if (sendLastPacket == -1) {
                printf("sendto() failed with error code  : %d", errno);
                free(packet);
                return -1;
            }
            sent_total += sendLastPacket;
        } while (ACKtimeOut(sockfd->socket_fd, packets_num, clock(), 1) <= 0);
        free(packet);
    }
    //printf("total byte sent is %ld\n", sent_total);
    return sent_total;
}

int rudp_disconnect(RUDP_Socket *sockfd){
    if(!sockfd->isConnected){
       printf("you are alredy discinnected\n");
        return 0;
    }
    struct sockaddr_in dest_addr = sockfd->dest_addr;

    ssize_t sent_len = sendto(sockfd->socket_fd, "EXIT", sizeof("EXIT"), 0,(struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (sent_len == -1) {
        perror("sendto() failed\n");
        return -1;  // Handle the error appropriately
    }

    sockfd->isConnected = false;
    printf("Disconnect message sent successfully\n");
    return 1;
}

int rudp_close(RUDP_Socket *sockfd){
    close(sockfd->socket_fd);
    free(sockfd);
    return 0;
}
