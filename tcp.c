#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "logger.h"

#define RECV_BUFFER 1024

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

int establish_connection(int sockfd, in_addr_t server_ip, uint16_t server_port)
{
    // setting up addr struct
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = server_ip;
    server_addr.sin_port = htons(server_port);

    // connecting to server
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        return -1;
    }
    LOGP("Connected to server.\n");

    return sockfd;
}

int bind_and_listen(int sockfd, uint16_t port)
{
    // setting up addr struct
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));

    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY; // binds local IP address
    my_addr.sin_port = htons(port);

    // binding socket to address
    if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0) {
        perror("Error binding socket to address");
        return -1;
    }

    // listening for client ~ with backlog 5
    if (listen(sockfd, 5) < 0) { 
        perror("Error listening for client");
        return -1;
    }
    LOG("Listening for connections on port %d\n", ntohs(my_addr.sin_port));

    return 1;
}

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
