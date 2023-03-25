#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#define RECV_BUFFER 1024

int create_udp_socket()
{
    int sockfd;
    if ((sockfd = socket(PF_INET, SOCK_DGRAM, PF_UNSPEC)) < 0) {
        perror("Error creating port");
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

struct sockaddr_in* get_addr_in(int port)
{
    // setting up addr struct
    struct sockaddr_in *my_addr = malloc(sizeof(struct sockaddr_in));
    if (my_addr == NULL) {
        perror("Error mallocing sockaddr_in");
        return NULL;
    }
    memset(my_addr, 0, sizeof(struct sockaddr_in));

    my_addr->sin_family = AF_INET;
    my_addr->sin_addr.s_addr = INADDR_ANY; // binds local IP address
    my_addr->sin_port = htons(port);

    return my_addr;
}

int bind_port(int sockfd, struct sockaddr_in *addr_in)
{
    // binding socket to address
    if (bind(sockfd, (struct sockaddr *) addr_in, sizeof(struct sockaddr_in)) < 0) {
        perror("Error binding socket to address");
        return -1;
    } else {
        printf("Binded to port %d\n", ntohs(addr_in->sin_port));
    }

    return 1;
}

int send_packet(int sockfd, char *payload, struct sockaddr_in *addr_in)
{
    int payload_len, to_len, bytes_sent;
    payload_len = strlen(payload);
    to_len = sizeof(struct sockaddr);
    if ((bytes_sent = sendto(sockfd, payload, payload_len, 0,
                        (struct sockaddr *) addr_in, to_len)) < 0) {
        perror("Error sending packet");
        return -1;
    } else {
        printf("Packet sent");
    }
    
    return 1;
}

char* receive_packet(int sockfd, struct sockaddr_in *addr_in)
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
                            (struct sockaddr *) addr_in, &from_len)) < 0) {
        perror("Error receiving packet");
        return NULL;
    }

    return buf;
}
