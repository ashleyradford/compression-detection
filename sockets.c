/**
 * @file
 *
 * Contains tcp, udp, and raw socket helper functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "logger.h"

#define RECV_BUFFER 1024

/**
 * Creates a sockaddr_in struct for the given ip and port numbers
 *
 * ip: char pointer to decimal ip address
 * port: port number
 *
 * returns: pointer to sockaddr_in struct if successful, NULL otherwise
 */
struct sockaddr_in* set_addr_struct(char* ip, uint16_t port)
{
    // setting up addr struct
    struct sockaddr_in *sin = malloc(sizeof(struct sockaddr_in));
    if (sin == NULL) {
        perror("Error mallocing sockaddr_in");
        return NULL;
    }
    memset(sin, 0, sizeof(struct sockaddr_in));

    sin->sin_family = AF_INET;
    sin->sin_port = htons(port);

    if (ip == NULL) {
        sin->sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, ip, &(sin->sin_addr)) != 1) {
            perror("Error converting IP address");
            return NULL;
        }
    }

    return sin;
}

/**
 * Creates raw socket and sets IP header flag so that
 * OS does not add their own headers
 *
 * returns: raw socket file descriptor if successful, -1 otherwise
 */
int create_raw_socket()
{
    int sockfd;
    if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP)) < 0) {
        perror("Error creating raw socket");
        return -1;
    }

    // set flag so socket expects our IPv4 header
    const int on = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
        perror ("Cannot set IP_HDRINCL option");
        return -1;
    }

    return sockfd;
}

/**
 * Adds timeout option to socket
 *
 * sockfd: socket file descriptor
 * wait_time: timeout time
 *
 * returns: socket file descriptor if successful, -1 otherwise
 */
int add_timeout_opt(int sockfd, int wait_time)
{
    struct timeval timeout;
    timeout.tv_sec = wait_time;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) == -1) {
        perror("Cannot add receive timeout");
        return -1;
    }

    return sockfd;
}

/**
 * Adds dont fragment bit option to socket
 *
 * sockfd: socket file descriptor
 *
 * returns: socket file descriptor if successful, -1 otherwise
 */
int set_df_opt(int sockfd)
{
    int df = IP_PMTUDISC_DO; // sets DF bit
    if (setsockopt(sockfd, SOL_SOCKET, IP_MTU_DISCOVER, &df, sizeof df) == -1) {
        perror("Cannot set MTU discovery");
        return -1;
    }

    return sockfd;
}

/**
 * Adds TTL option to socket
 *
 * sockfd: socket file descriptor
 * ttl: time to live
 *
 * returns: socket file descriptor if successful, -1 otherwise
 */
int add_ttl_opt(int sockfd, int ttl)
{
    if (setsockopt(sockfd, SOL_SOCKET, IP_TTL, &ttl, sizeof ttl) == -1) {
        perror("Cannot set TTL");
        return -1;
    }

    return sockfd;
}

// ------------------- TCP Specific Functions ------------------- //

/**
 * Creates tcp socket and sets reuseaddr option to prevent
 * address already in use error
 *
 * returns: tcp socket file descriptor if successful, -1 otherwise
 */
int create_tcp_socket()
{
    int sockfd;
    if ((sockfd = socket(PF_INET, SOCK_STREAM, PF_UNSPEC)) < 0) {
        perror("Error creating tcp socket");
        return -1;
    }

    // for the address already in use error
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        perror("Cannot reuse port");
        return -1;
    }

    return sockfd;
}

/**
 * Establishes a tcp connection
 *
 * sockfd: tcp socket file descriptor
 * server_ip: char pointer to decimal ip address
 * server_port: tcp port number
 *
 * returns: tcp socket file descriptor if successful, -1 otherwise
 */
int establish_connection(int sockfd, char* server_ip, uint16_t server_port)
{
    // setting up addr struct
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &(server_addr.sin_addr)) != 1) {
            perror("Error converting IP address");
            return -1;
    }

    // connecting to server
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        return -1;
    }
    LOGP("Connected to server.\n");

    return sockfd;
}

/**
 * Binds tcp socket and listens for incoming connections
 *
 * sockfd: tcp socket file descriptor
 * port: tcp port number
 *
 * returns: 1 if successful, -1 otherwise
 */
int bind_and_listen(int sockfd, uint16_t port)
{
    // setting up addr struct
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY; // binds local IP address
    sin.sin_port = htons(port);

    // binding socket to address
    if (bind(sockfd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        perror("Error binding socket to address");
        return -1;
    }

    // listening for client ~ with backlog 5
    if (listen(sockfd, 5) < 0) { 
        perror("Error listening for client");
        return -1;
    }
    LOG("Listening for connections on port %d\n", ntohs(sin.sin_port));

    return 1;
}

/**
 * Accepts an incoming tcp connection
 *
 * sockfd: tcp socket file descriptor
 *
 * returns: new accepted socket file descriptor if successful, -1 otherwise
 */
int accept_connection(int sockfd)
{
    struct sockaddr_in new_addr;
    uint new_addr_len = sizeof(new_addr);

    int new_sock = accept(sockfd, (struct sockaddr *) &new_addr, &new_addr_len);
    if (new_sock < 0) {
        perror("Error accepting connections");
        return -1;
    }
    LOGP("Connection accepted.\n");

    return new_sock;
}

/**
 * Sends data stream across tcp connection
 *
 * sockfd: tcp socket file descriptor
 * msg: char pointer to data stream
 *
 * returns: 1 if successful, -1 otherwise
 */
int send_stream(int sockfd, char *msg)
{
    int len, bytes_sent;
    len = strlen(msg);
    if ((bytes_sent = send(sockfd, msg, len, 0)) < 0) {
        perror("Error sending message");
        return -1;
    }

    return 1;
}

/**
 * Receives data stream across tcp connection
 *
 * sockfd: tcp socket file descriptor
 *
 * returns: char pointer to received data if successful, NULL otherwise
 */
char* receive_stream(int sockfd)
{
    char *buf = malloc(RECV_BUFFER);
    if (buf == NULL) {
        perror("Error mallocing buf");
        return NULL;
    }
    memset(buf, '\0', RECV_BUFFER);

    int bytes_received;
    if ((bytes_received = recv(sockfd, buf, RECV_BUFFER, 0)) < 0) {
        perror("Error receiving bytes");
        return NULL;
    }

    return buf;
}

// ------------------- UDP Specific Functions ------------------- //

/**
 * Creates udp socket
 *
 * returns: udp socket file descriptor if successful, -1 otherwise
 */
int create_udp_socket()
{
    int sockfd;
    if ((sockfd = socket(PF_INET, SOCK_DGRAM, PF_UNSPEC)) < 0) {
        perror("Error creating udp socket");
        return -1;
    }

    return sockfd;
}

/**
 * Binds udp socket
 *
 * sockfd: udp socket file descriptor
 * sin: pointer to sockaddr_in struct for udp port
 *
 * returns: 1 if successful, -1 otherwise
 */
int bind_port(int sockfd, struct sockaddr_in *sin)
{
    // binding socket to address
    if (bind(sockfd, (struct sockaddr *) sin , sizeof(struct sockaddr)) < 0) {
        perror("Error binding socket to address");
        return -1;
    }
    LOG("Binded to port %d for incoming UDP packets.\n", ntohs(sin->sin_port));

    return 1;
}

/**
 * Sends udp datagram to specified address
 *
 * sockfd: udp socket file descriptor
 * packet: char pointer to packet
 * packet_size: packet size
 * sin: pointer to sockaddr_in struct to send datagram to
 *
 * returns: 1 if successful, -1 otherwise
 */
int send_packet(int sockfd, char *packet, int packet_size, struct sockaddr_in *sin)
{
    int to_len, bytes_sent;
    to_len = sizeof(struct sockaddr);
    
    if ((bytes_sent = sendto(sockfd, packet, packet_size, 0,
                        (struct sockaddr *) sin, to_len)) < 0) {
        perror("Error sending packet");
        return -1;
    }

    // LOG("Packet sent to %d.\n", ntohs(sin->sin_port));

    return 1;
}

/**
 * Receives udp datagram
 *
 * sockfd: udp socket file descriptor
 * sin: pointer to sockaddr_in struct to be filled
 *
 * returns: char pointer to received data if successful, NULL otherwise
 */
char* receive_packet(int sockfd, struct sockaddr_in *sin)
{
    char *buf = malloc(RECV_BUFFER);
    if (buf == NULL) {
        perror("Error mallocing buf");
        return NULL;
    }
    memset(buf, '\0', RECV_BUFFER);

    int bytes_received;
    uint from_len = sizeof(struct sockaddr);
    if ((bytes_received = recvfrom(sockfd, buf, RECV_BUFFER, 0,
                            (struct sockaddr *) sin, &from_len)) < 0) {
        if (errno == 11) {
            return NULL;
        }
        perror("Error receiving packet");
        return NULL;
    }

    // LOG("Bytes recieved: %d\n", bytes_received);
    // LOG("Received from ip: %u\n", ntohl(sin->sin_addr.s_addr));
    // LOG("Received from port: %d\n", ntohs(sin->sin_port));

    return buf;
}
