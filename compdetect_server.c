#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "cJSON.h"

void print_usage(char *argv[])
{
    fprintf(stderr, "Usage: %s port\n", argv[0]);
}

int receive_config(unsigned short port)
{
    // setting up addr struct
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));

    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY; // binds local IP address
    my_addr.sin_port = htons(port);

    // creating a socket
    int sockfd = socket(PF_INET, SOCK_STREAM, PF_UNSPEC);

    // binding port to socket
    if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0) {
        perror("Error binding socket to address");
        return -1;
    }

    // listeing for client ~ with backlog 5
    if (listen(sockfd, 5) < 0) { 
        perror("Error listening for client");
        return -1;
    }

    // accepting incoming client connection
    struct sockaddr_in new_addr;
    uint new_addr_len = sizeof(new_addr);
    
    int new_sock = accept(sockfd, (struct sockaddr *) &new_addr, &new_addr_len);
    if (new_sock < 0) {
        perror("Error accepting connection");
        return -1;
    }

    // communicating with client
    char buffer[1025];
    memset(&buffer, '\0', 1025);
    int bytes_received;
    if ((bytes_received = recv(new_sock, buffer, 1025, 0)) < 0) {
        perror("Error receiving bytes");
        return -1;
    }
    fprintf(stderr, "Bytes read: %d\n", bytes_received);
    fprintf(stderr, "Message received: %s", buffer);

    return 1;
}

int main(int argc, char *argv[])
{
    // check that port is provided
    if (argc < 2) {
        print_usage(argv);
        return EXIT_FAILURE;
    }

    unsigned short listen_port = atoi(argv[1]);

    // pre probing phase
    if (receive_config(listen_port) < 0) {
        return EXIT_FAILURE;
    }

    // probing phase


    // post probing phase


    return EXIT_SUCCESS;
}
