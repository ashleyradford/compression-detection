#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <stdbool.h>
#include <string.h>

#include "cJSON.h"
#include "tcp.h"
#include "udp.h"
#include "logger.h"

#define RECV_BUFFER 1024

struct server_config {
    uint16_t udp_dest_port;
    int udp_train_size;
    int udp_timeout;
    int threshold;
};

void parse_config(struct server_config *configs, char *contents)
{
    // parse json
    cJSON *root = cJSON_Parse(contents);
    configs->udp_dest_port = atoi(cJSON_GetObjectItem(root, "udp_dest_port")->valuestring);
    configs->udp_train_size = atoi(cJSON_GetObjectItem(root, "udp_train_size")->valuestring);
    configs->udp_timeout = atoi(cJSON_GetObjectItem(root, "udp_timeout")->valuestring);
    configs->threshold = atoi(cJSON_GetObjectItem(root, "threshold")->valuestring);
}

double time_diff(struct timeval tv1, struct timeval tv2)
{
    double tv1_mili = (tv1.tv_sec * 1000) + (tv1.tv_usec / 1000);
    double tv2_mili = (tv2.tv_sec * 1000) + (tv2.tv_usec / 1000);
    return tv1_mili - tv2_mili;
}

struct server_config* pre_probing(uint16_t listen_port)
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

    LOGP("Config contents received, closing TCP connection.\n");
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
    if ((sock = create_udp_socket()) < 0) {
        return NULL;
    }

    // add timeout
    if (add_timeout(sock, configs->udp_timeout) < 0) {
        return NULL;
    }

    // set up addr struct and bind port
    struct sockaddr_in *my_addr = get_addr_in(INADDR_ANY, configs->udp_dest_port);
    if (bind_port(sock, my_addr) < 0) {
        return NULL;
    }

    struct timeval low_start, low_end, high_start, high_end;
    char* payload;
    bool begin = false;

    // receive low entropy packets   
    for (int i = 0; i < configs->udp_train_size; i++) {
        if ((payload = receive_packet(sock, my_addr)) == NULL) {
            if (errno == 11) { // EAGAIN
                break;
            }
            return NULL;
        }
        if (!begin) {
            gettimeofday(&low_start, NULL);
            begin = true;
        }
        gettimeofday(&low_end, NULL);
    }
    LOGP("First train received.\n");

    // receive high entropy packets
    begin = false;
    for (int i = 0; i < configs->udp_train_size; i++) {
        if ((payload = receive_packet(sock, my_addr)) == NULL) {
            if (errno == 11) { // EAGAIN
                break;
            }
            return NULL;
        }
        if (!begin) {
            gettimeofday(&high_start, NULL);
            begin = true;
        }
        gettimeofday(&high_end, NULL);
    }
    LOGP("Second train received.\n");

    // congestion detection calculations
    char *result;
    double low_delta = time_diff(low_end, low_start);
    double high_delta = time_diff(high_end, high_start);
    double difference = high_delta - low_delta;

    LOG("Low entropy: %.0fms\n", low_delta);
    LOG("High entropy: %.0fms\n", high_delta);
    LOG("Delta: %.0fms\n", difference);

    if (difference > configs->threshold) {
        result = "Compression detected.";
    } else {
        result = "No compression detected.";
    }

    free(my_addr);
    free(payload);

    return result;
}

int post_probing(uint16_t listen_port, char *msg)
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

    LOGP("Congestion status sent, closing TCP connection.\n");
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

    uint16_t listen_port = atoi(argv[1]);

    // ---- pre probing phase ----
    struct server_config *configs;
    if ((configs = pre_probing(listen_port)) == NULL) {
        return EXIT_FAILURE;
    }

    // ---- probing phase ----
    char *results;
    if ((results = probing(configs)) == NULL) {
        return EXIT_FAILURE;
    }

    // ---- post probing phase ----
    if (post_probing(listen_port, results) < 0) {
        return EXIT_FAILURE;
    }

    // free config structure data
    free(configs);

    return EXIT_SUCCESS;
}
