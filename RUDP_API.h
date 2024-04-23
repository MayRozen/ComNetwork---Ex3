#pragma once
#include <stdlib.h>
#include <stdbool.h>
#define MAX_SIZE  60000
// A struct that represents RUDP Socket
typedef struct _rudp_socket{
    int socket_fd; // UDP socket file descriptor
    bool isServer; // True if the RUDP socket acts like a server, false for client.
    bool isConnected; // True if there is an active connection, false otherwise.
    struct sockaddr_in dest_addr; // Destination address. Client fills it when it connects via rudp_connect(), server fills it when it accepts a connection via rudp_accept().
} RUDP_Socket;

typedef struct _flags {
  unsigned int SYN : 1;
  unsigned int ACK : 1;
  unsigned int DATA : 1;
  unsigned int FIN : 1;
} Flags;

typedef struct rudp_header{
    Flags flag;
    int length;
    int checksum;
    int seqNum;
}Header;

typedef struct packet {
  Header header;
  char data[MAX_SIZE];
} Packet;

unsigned short int calculate_checksum(void *data, unsigned int bytes);

int timeoutSeting(int socket, int time);
  
//char *util_generate_random_data(unsigned int size);

// Allocates a new structure for the RUDP socket (contains basic information about the socket itself).
// Also creates a UDP socket as a baseline for the RUDP. isServer means that this socket acts like a server. 
// If set to server socket, it also binds the socket to a specific port.
RUDP_Socket* rudp_socket(bool isServer, unsigned short int listen_port);

// Tries to connect to the other side via RUDP to given IP and port. Returns 0 on failure and 1 on success.
// Fails if called when the socket is connected/set to server.
int rudp_connect(RUDP_Socket *sockfd, const char *dest_ip, unsigned short int dest_port);

// Accepts incoming connection request and completes the handshake, returns 0 on failure and 1 on success.
// Fails if called when the socket is connected/set to client.
int rudp_accept(RUDP_Socket *sockfd);

// Receives data from the other side and put it into the buffer. Returns the number of received bytes on success,
// 0 if got FIN packet (disconnect), and -1 on error. Fails if called when the socket is disconnected.
int rudp_recv(RUDP_Socket *sockfd, void **buffer, int *buffer_size);

// Sends data stores in buffer to the other side. Returns the number of sent bytes on success,
// 0 if got FIN packet (disconnect), and -1 on error. Fails if called when the socket is disconnected.
int rudp_Send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size);

// Disconnects from an actively connected socket. Returns 1 on success, 0 when the socket is already disconnected (failure).
int rudp_disconnect(RUDP_Socket *sockfd);

// This function releases all the memory allocation and resources of the socket.
int rudp_close(RUDP_Socket *sockfd);