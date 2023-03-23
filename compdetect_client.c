#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>

void print_usage(char *argv[])
{
    fprintf(stderr, "Usage: %s config_file.json\n", argv[0]);
}

// int send_config(*char config_data, uint32_t server_ip, unsigned short server_port)
int send_config(in_addr_t server_ip, unsigned short server_port)
{
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
        return 1;
    }

    // sending config data to server
    char *msg = "Hello I am the client";
    fprintf(stderr, "Message sent: %s\n", msg);
    int len, bytes_sent;
    len = strlen(msg);
    bytes_sent = send(sockfd, msg, len, 0);
}

int main(int argc, char *argv[])
{
    // check that config file is provided
    if (argc < 2) {
        print_usage(argv);
        return EXIT_FAILURE;
    }

    // parse config file
    // ERROR CHECK
    in_addr_t server_ip = inet_addr("192.168.64.9");
    unsigned short server_port = 8787;
    

    // pre probing phase
    if (send_config(server_ip, server_port) < 0) {
        return EXIT_FAILURE;
    }

    // probing phase


    // post probing phase


    return EXIT_SUCCESS;
}
