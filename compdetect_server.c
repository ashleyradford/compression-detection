#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "cJSON.h"
#include "tcp.h"
#include "udp.h"

#define RECV_BUFFER 1024
#define TIMEOUT_MULTI 4

struct server_config {
    unsigned short udp_dest_port;
    int udp_payload_size;
    int inter_measurement_time;
    int udp_train_size;
};

void parse_config(struct server_config *configs, char *contents)
{
    // parse json
    cJSON *root = cJSON_Parse(contents);
    configs->udp_dest_port = atoi(cJSON_GetObjectItem(root, "udp_dest_port")->valuestring);
    configs->udp_payload_size = atoi(cJSON_GetObjectItem(root, "udp_payload_size")->valuestring);
    configs->inter_measurement_time = atoi(cJSON_GetObjectItem(root, "inter_measurement_time")->valuestring);
    configs->udp_train_size = atoi(cJSON_GetObjectItem(root, "udp_train_size")->valuestring);
}

struct server_config* pre_probing(unsigned short listen_port)
{
    // bind port and accept client connection
    int sock;
    if ((sock = create_tcp_socket()) < 0) {
        return NULL;
    }

    if (bind_and_listen(sock, listen_port) < 0) {
        return NULL;
    }
    
    int new_sock;
    if ((new_sock = accept_connection(sock)) < 0) {
        return NULL;
    }

    // receive message
    char *config_contents;
    if ((config_contents = receive_stream(new_sock)) == NULL) {
        return NULL;
    }

    printf("Config contents received, closing TCP connection.\n");
    if (close(new_sock) < 0 || close(sock) < 0) {
        perror("Error closing socket");
        return NULL;
    }

    // parse received config file
    struct server_config *configs = malloc(sizeof(struct server_config));
    parse_config(configs, config_contents);
    free(config_contents);

    return configs;
}

char* probing(struct server_config *configs)
{
    int sock;
    if ((sock = create_udp_socket(configs->inter_measurement_time / TIMEOUT_MULTI)) < 0) {
        return NULL;
    }

    // set up addr struct and bind port
    struct sockaddr_in *my_addr = get_addr_in(INADDR_ANY, configs->udp_dest_port);
    if (bind_port(sock, my_addr) < 0) {
        return NULL;
    }

    // receive packets
    char* packet;
    while(1) {
        if ((packet = receive_packet(sock, my_addr)) == NULL) {
            if (errno == 11) {
                break;
            }
            return NULL;
        }
    }

    // calculations ~ shorter than inter time

    free(my_addr);

    return "Hellooo";
}

int post_probing(unsigned short listen_port, char *msg)
{
    // bind port and accept client connection
    int sock;
    if ((sock = create_tcp_socket()) < 0) {
        return -1;
    }

    if (bind_and_listen(sock, listen_port) < 0) {
        return -1;
    }

    int new_sock;
    if ((new_sock = accept_connection(sock)) < 0) {
        return -1;
    }

    // send congestion results
    if (send_stream(new_sock, msg) < 0) {
        return -1;
    }

    printf("Congestion status sent, closing TCP connection.\n");
    if (close(new_sock) < 0 || close(sock) < 0) {
        perror("Error closing socket");
        return -1;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    // check that port is provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        return EXIT_FAILURE;
    }

    unsigned short listen_port = atoi(argv[1]);

    // pre probing phase
    struct server_config *configs;
    if ((configs = pre_probing(listen_port)) == NULL) {
        return EXIT_FAILURE;
    }

    // probing phase
    char *results;
    if ((results = probing(configs)) == NULL) {
        return EXIT_FAILURE;
    }

    // post probing phase
    // if (post_probing(listen_port, results) < 0) {
    //     return EXIT_FAILURE;
    // }

    // free config structure data
    free(configs);

    return EXIT_SUCCESS;
}
