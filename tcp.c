#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define RECV_BUFFER 1024

struct server_config {
    unsigned short udp_dest_port;
    int udp_payload_size;
};

struct client_config {
    in_addr_t server_ip;
    unsigned short udp_dest_port;
    unsigned short tcp_port;
    int udp_payload_size;
    int inter_measurement_time;
    int udp_train_size;
    int ttl;
};

int create_socket() {
    int sockfd = socket(PF_INET, SOCK_STREAM, PF_UNSPEC);

    // for the address already in use error
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        perror("Cannot resuse port");
        return -1;
    }

    return sockfd;
}

int establish_connection(in_addr_t server_ip, unsigned short server_port) {
    // creating a socket
    int sockfd = socket(PF_INET, SOCK_STREAM, PF_UNSPEC);

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
    } else {
        printf("Connected to server.\n");
    }

    return sockfd;
}

int bind_and_listen(int sockfd, unsigned short port)
{
    // setting up addr struct
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));

    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY; // binds local IP address
    my_addr.sin_port = htons(port);

    // binding port to socket
    if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0) {
        perror("Error binding socket to address");
        return -1;
    }

    // listening for client ~ with backlog 5
    if (listen(sockfd, 5) < 0) { 
        perror("Error listening for client");
        return -1;
    } else {
        printf("Listening for connections...\n");
    }

    return 1;
}

int accept_connection(int sockfd)
{
    // accepting incoming client connection
    struct sockaddr_in new_addr;
    uint new_addr_len = sizeof(new_addr);

    int new_sock = accept(sockfd, (struct sockaddr *) &new_addr, &new_addr_len);
    if (new_sock < 0) {
        perror("Error accepting connection");
        return -1;
    } else {
        printf("Connection accepted.\n");
    }

    return new_sock;
}

char* receive_msg(int sockfd)
{
    char *buf = malloc(RECV_BUFFER);
    memset(buf, '\0', RECV_BUFFER);

    int bytes_received;
    if ((bytes_received = recv(sockfd, buf, RECV_BUFFER, 0)) < 0) {
        perror("Error receiving bytes");
        return NULL;
    }

    return buf;
}

int send_msg(int sockfd, char *msg)
{
    int len, bytes_sent;
    len = strlen(msg);
    if ((bytes_sent = send(sockfd, msg, len, 0)) < 0) {
        perror("Error sending message");
        return -1;
    }
    
    return 1;
}
