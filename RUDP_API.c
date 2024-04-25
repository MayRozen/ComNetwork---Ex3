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

int sqNum = 0;

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
    pPacket pack = (pPacket)malloc(sizeof(packet));
    while ((double)(clock() - start) / CLOCKS_PER_SEC < timeout) {
        //int recvLen = recvfrom(socket, pack, sizeof(packet)-1, 0, NULL, 0);
        // if (recvLen == -1) {
        //     free(pack);
        //     return 0;
        // }
        if (pack->header.seqNum == seqNumber && pack->header.flag.ACK == 1) {
            free(pack);
            return 1;
        }
    }
  free(pack);
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
            free(sockfd);
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
    pPacket pack = (pPacket)malloc(sizeof(packet));
    memset(pack, 0, sizeof(packet));
    pack->header.flag.SYN = 1;//SYN
    int total_tries = 0;
    pPacket recv_packet = NULL;
    while (total_tries < 3) {
        if (sendto(sockfd->socket_fd, pack, sizeof(packet), 0, (struct sockaddr*)&sockfd->dest_addr, sizeof(sockfd->dest_addr)) == -1) {
            perror("Error sending connection request");
            free(pack);
            return -1;
        }
        clock_t startingTime = clock();
        do {
            // receive SYN-ACK message
            recv_packet = malloc(sizeof(packet));
            memset(recv_packet, 0, sizeof(packet));
            struct sockaddr_in sender_address;
            socklen_t sender_address_len = sizeof(sender_address);

            recvBytes = recvfrom(sockfd->socket_fd, recv_packet, sizeof(packet), 0, (struct sockaddr *)&sender_address, &sender_address_len);

            //recvBytes = recvfrom(sockfd->socket_fd, recv_packet, sizeof(packet), 0, NULL, 0);
            if (recvBytes == -1) {//problom!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                perror("recvfrom failed");
                return -1;
            }
            if (recv_packet->header.flag.SYN == 1 && recv_packet->header.flag.ACK == 1) {
                printf("connected\n");
                free(pack);
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
    free(pack);
    free(recv_packet);
    return 0;
}

int rudp_accept(RUDP_Socket *sockfd){
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);
    memset((char *)&clientAddress, 0, sizeof(clientAddress));
    
    pPacket pack = malloc(sizeof(packet));
    memset(pack, 0, sizeof(packet));
    int recv_len = recvfrom(sockfd->socket_fd, pack, sizeof(packet) - 1, 0,(struct sockaddr *)&clientAddress, &clientAddressLen);
    if (recv_len == -1) {
        perror("recvfrom() failed on receiving SYN");
        free(pack);
        return -1;
    }

    if (connect(sockfd->socket_fd, (struct sockaddr *)&clientAddress, clientAddressLen) == -1) {
        perror("connect() failed with error code");
        free(pack);
        return -1;
    }

    if (pack->header.flag.SYN == 1) {
        printf("received SYN request\n");
        pPacket packetReply = (pPacket)malloc(sizeof(packet));
        memset(pack, 0, sizeof(packet));
        packetReply->header.flag.SYN = 1;
        packetReply->header.flag.ACK = 1;
        if (sendto(sockfd->socket_fd, packetReply, sizeof(packet), 0, NULL, 0) == -1) {
            printf("sendto() failed with error code  : %d", errno);
            free(pack);
            free(packetReply);
            return -1;
        }
        timeoutSeting(sockfd->socket_fd, 1 * 10);
        free(pack);
        free(packetReply);
        sockfd->isConnected = true;
        printf("rudp_accept() successeded\n");
        return 1;
    }
    return 0;
}

int rudp_recv(RUDP_Socket *sockfd, void *buffer, int buffer_size){
    if (sockfd == NULL) {//if the socket isn't valid
        perror("Invalid RUDP socket");
        return -1;
    }
    if (!sockfd->isConnected) {//if the sender is not connected
        printf("Receive failed. Socket is not connected\n");
        return -1;
    }
    
    pPacket pack = (pPacket)malloc(sizeof(packet));
    if(pack == NULL){
        printf("malloc failed\n");
        free(pack);
        return -1;
    }
    memset(pack, 0, sizeof(packet));
    int recv_len = recvfrom(sockfd->socket_fd, pack, sizeof(packet) , 0, NULL, 0);
    if (recv_len == -1) {
        perror("recvfrom() failed test");
        free(pack);
        return -1;
    }
    
    int checksum = calculate_checksum(pack->data,pack->header.length);
    // check if the packet is corrupted, and send ack
    if (checksum != pack->header.checksum) {
        printf("checksum != pack->header.checksum\n");
        free(pack);
        return -1;
    }
    printf("the sq number is: %d\n", sqNum);
    printf("the header sq number is: %d\n",pack->header.seqNum);
    if (pack->header.seqNum == sqNum) {
        if (pack->header.seqNum == 0 && pack->header.flag.DATA == 1) {
            timeoutSeting(sockfd->socket_fd, 1);//ten seconds
        }
        if (pack->header.flag.ACK == 1 && pack->header.flag.SYN == 1) {  // last packet
            buffer = malloc(pack->header.length);  // Allocate memory for data
            if(buffer == NULL){
                printf("malloc failed\n");
                free(pack);
                return -1;
            }
            memcpy(buffer, pack->data, pack->header.length);
            buffer_size = pack->header.length;
            timeoutSeting(sockfd->socket_fd, 1);
            free(pack);
            free(buffer);
            return recv_len;
        }
        if (pack->header.flag.DATA == 1) {     // data packet
            buffer = malloc(pack->header.length);  // Allocate memory for data
            if(buffer == NULL){
                printf("malloc failed\n");
                free(pack);
                return -1;
            }
            memcpy(buffer, pack->data, pack->header.length);
            buffer_size = pack->header.length;
            free(pack);
            free(buffer);
            sqNum++;
            return recv_len;
        }
    } 
    else if (pack->header.flag.DATA == 1) {
        free(pack);
        return -1;
    }

    if(sockfd->isServer){
        if(pack->header.flag.FIN == 1){
            free(pack);
            timeoutSeting(sockfd->socket_fd,10);
            pack = (pPacket)malloc(sizeof(packet));
            if(pack == NULL){
                printf("malloc failed\n");
                free(pack);
                return -1;
            }
            time_t finishTime = time(NULL);
            while ((double)(time(NULL) - finishTime) < 1) {
                memset(pack, 0, sizeof(packet));
                recv_len = recvfrom(sockfd->socket_fd, pack, sizeof(packet), 0, NULL, 0);
                if (recv_len == -1) {
                    perror("recvfrom() failed");
                    free(pack);
                    return -1;
                }
                if (pack->header.flag.FIN == 1) {
                    pPacket ack = (pPacket)malloc(sizeof(packet));
                    if(ack == NULL){
                        printf("malloc failed\n");
                        free(pack);
                        return -1;
                    }
                    memset(ack, 0, sizeof(packet));
                    ack->header.flag.ACK = 1;
                    if (pack->header.flag.FIN == 1){
                        ack->header.flag.FIN = 1;
                    }
                    if (pack->header.flag.SYN == 1){
                        ack->header.flag.SYN = 1;
                    }
                    if (pack->header.flag.DATA == 1){
                        ack->header.flag.DATA = 1;
                    }
                    ack->header.seqNum = pack->header.seqNum;
                    ack->header.checksum = calculate_checksum(pack->data,pack->header.length);
                    int sendResult = sendto(sockfd->socket_fd, ack, sizeof(packet), 0, NULL, 0);
                    if (sendResult == -1) {
                        perror("sendto() failed");
                        free(ack);
                        free(pack);
                        return -1;
                    }
                    free(ack);
                    finishTime = time(NULL);
                }
                free(pack);
                rudp_disconnect(sockfd);
                printf("test");
                return 0;
            }
        }
    }
    else{
        free(pack);
    }
    printf("received %d byte\n",recv_len);
    return -1;
}

int rudp_Send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size){
    if (sockfd == NULL) {
        fprintf(stderr, "Invalid RUDP socket\n");
        return 0;
    }

    if (!sockfd->isConnected){
        perror("Socket is not connected");
        rudp_close(sockfd);
        return -1;
    }
    // calculate the number of packets needed to send the data
    int packets_num = buffer_size / MAX_SIZE;
    // calculate the size of the last packet
    int last_packet_size = buffer_size % MAX_SIZE;
    pPacket pack = (pPacket)malloc(sizeof(packet));
    if(pack == NULL){
        printf("malloc failed\n");
        return -1;
    }
    int sent_len = 0;
    int sent_total = 0;
    // send the packets
    for (int i = 0; i < packets_num; i++){
        memset(pack, 0, sizeof(packet));
        pack->header.seqNum = i;     // set the sequence number
        pack->header.flag.DATA = 1;  // set the DATA flag
        // if we are at the last packet, we will set the FIN flag to 1
        if (i == packets_num - 1 && last_packet_size == 0) {
            pack->header.flag.FIN = 1;
        }
        // copy the data in the packet to a buffer
        memcpy(pack->data, buffer + i * MAX_SIZE, MAX_SIZE);
        pack->header.length = MAX_SIZE;
        // calculate the checksum of the packet before the actual sending
        pack->header.checksum = calculate_checksum(pack->data,pack->header.length);

       // do {
        // ssize_t chunk_size = remaining > MAX_UDP_PAYLOAD_SIZE ? MAX_UDP_PAYLOAD_SIZE : remaining;
            sent_len = sendto(sockfd->socket_fd, pack, sizeof(packet), 0, (struct sockaddr *)&(sockfd->dest_addr), sizeof(sockfd->dest_addr));
            if (sent_len == -1) {
                perror("sendto() failed");
                return -1;  // Handle the error appropriately
            }
       // }while (ACKtimeOut(sockfd->socket_fd, i, clock(), 1) <= 0);
        sent_total += sent_len;
    }
    if (last_packet_size > 0) {
        memset(pack, 0, sizeof(packet));
        // set the fields of the packet
        pack->header.seqNum = packets_num;
        pack->header.flag.DATA = 1;
        pack->header.flag.FIN = 1;
        memcpy(pack->data, buffer + packets_num * MAX_SIZE, last_packet_size);
        pack->header.length = last_packet_size;
        pack->header.checksum = calculate_checksum(pack->data,pack->header.length);
        int sendLastPacket = sendto(sockfd->socket_fd, pack, sizeof(packet), 0, NULL, 0);
        if (sendLastPacket == -1) {
            perror("sendto() failed");
            free(pack);
            return -1;
        }
        sent_total += sendLastPacket;
        free(pack);
    }
    printf("total byte sent is %d\n", sent_total);
    return sent_total;
}

int rudp_disconnect(RUDP_Socket *sockfd){
    if(!sockfd->isConnected){
       printf("you are already discinnected\n");
        return 0;
    }
    sockfd->isConnected = false;
    //close(sockfd->socket_fd);
    printf("Disconnected successfully\n");
    return 1;
}

int rudp_close(RUDP_Socket *sockfd){
    close(sockfd->socket_fd);
    free(sockfd);
    return 0;
}