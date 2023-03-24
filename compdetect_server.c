#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "cJSON.h"
#include "tcp.h"

#define RECV_BUFFER 1024

struct server_config {
    unsigned short udp_dest_port;
    int udp_payload_size;
};

void parse_config(struct server_config *configs, char *contents)
{
    // parse json
    cJSON *root = cJSON_Parse(contents);
    configs->udp_dest_port = atoi(cJSON_GetObjectItem(root, "udp_dest_port")->valuestring);
    configs->udp_payload_size = atoi(cJSON_GetObjectItem(root, "udp_payload_size")->valuestring);
}

int main(int argc, char *argv[])
{
    // check that port is provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        return EXIT_FAILURE;
    }

    // --- PRE PROBING PHASE ---
    unsigned short listen_port = atoi(argv[1]);

    // bind port and accept client connection
    int sock;
    if ((sock = create_socket()) < 0) {
        return EXIT_FAILURE;
    }

    if (bind_and_listen(sock, listen_port) < 0) {
        return EXIT_FAILURE;
    }
    
    int new_sock;
    if ((new_sock = accept_connection(sock)) < 0) {
        return EXIT_FAILURE;
    }

    // receive message
    char *config_contents;
    if ((config_contents = receive_msg(new_sock)) == NULL) {
        return EXIT_FAILURE;
    }

    printf("Config contents received, closing TCP connection.\n");
    close(new_sock);
    close(sock);

    // parse received config file
    struct server_config *configs = malloc(sizeof(struct server_config));
    parse_config(configs, config_contents);

    // --- PROBING PHASE ---

    // use a short int
    // incremenet using bit wise op
    // move most sig and lest sig

    // using clock
    // get time of day (better one to use)
    sleep(4);

    // --- POST PROBING PHASE ---
    char *msg = "It's me, hi im the problem\n";
    if ((sock = create_socket()) < 0) {
        perror("Error creating port");
        return EXIT_FAILURE;
    }

    if (bind_and_listen(sock, listen_port) < 0) {
        perror("Error binding port");
        return EXIT_FAILURE;
    }
    
    if ((new_sock = accept_connection(sock)) < 0) {
        perror("Error accepting connection");
        return EXIT_FAILURE;
    }

    if (send_msg(new_sock, msg) < 0) {
        return EXIT_FAILURE;
    }

    printf("Congestion status sent, closing TCP connection.\n");
    close(new_sock);
    close(sock);

    // free memory
    free(config_contents);
    free(configs);

    return EXIT_SUCCESS;
}
