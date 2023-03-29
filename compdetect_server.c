/**
 * @file
 *
 * Server application that detects compression in a cooperative environment.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/time.h>
#include <netinet/in.h>

#include "cJSON.h"
#include "sockets.h"
#include "util.h"
#include "logger.h"

#define RECV_BUFFER 1024

struct server_config {
    uint16_t udp_dest_port;
    int udp_train_size;
    int udp_timeout;
    int threshold;
};

/**
 * Parses JSON file for server specific configurations
 *
 * configs: server_config struct to fill
 * contents: json text to parse
 */
void parse_config(struct server_config *configs, char *contents)
{
    cJSON *root = cJSON_Parse(contents);
    configs->udp_dest_port = atoi(cJSON_GetObjectItem(root, "udp_dest_port")->valuestring);
    configs->udp_train_size = atoi(cJSON_GetObjectItem(root, "udp_train_size")->valuestring);
    configs->udp_timeout = atoi(cJSON_GetObjectItem(root, "udp_timeout")->valuestring);
    configs->threshold = atoi(cJSON_GetObjectItem(root, "threshold")->valuestring);
}

/**
 * Pre-probing phase of compression detection. Accepts a
 * TCP connection and receives configuration data.
 *
 * listen_port: port to listen on 
 *
 * returns: server_config struct parsed from received data
 */
struct server_config* pre_probing(uint16_t listen_port)
{
    // bind port and accept client connection
    int tcp_sock;
    if ((tcp_sock = create_tcp_socket()) < 0) {
        return NULL;
    }

    if (bind_and_listen(tcp_sock, listen_port) < 0) {
        return NULL;
    }
    
    int client_sock;
    if ((client_sock = accept_connection(tcp_sock)) < 0) {
        return NULL;
    }

    // receive message
    char *config_contents;
    if ((config_contents = receive_stream(client_sock)) == NULL) {
        return NULL;
    }

    LOGP("Config contents received, closing TCP connection.\n");
    if (close(tcp_sock) < 0) {
        perror("Error closing tcp socket");
        return NULL;
    }
    if (close(client_sock) < 0) {
        perror("Error closing client socket");
        return NULL;
    }

    // parse received config file
    struct server_config *configs = malloc(sizeof(struct server_config));
    parse_config(configs, config_contents);
    free(config_contents);

    return configs;
}

/**
 * Probing phase of compression detection. Receives two sets of
 * UDP packets back to back, one with low entropy and one with
 * high entropy.
 *
 * configs: pointer to server_config struct
 *
 * returns: compression results if successful, NULL otherwise
 */
char* probing(struct server_config *configs)
{
    int udp_sock;
    if ((udp_sock = create_udp_socket()) < 0) {
        return NULL;
    }

    // add timeout
    if (add_timeout_opt(udp_sock, configs->udp_timeout) < 0) {
        return NULL;
    }

    // set up addr struct and bind port
    struct sockaddr_in *my_addr = set_addr_struct(INADDR_ANY, configs->udp_dest_port);
    if (bind_port(udp_sock, my_addr) < 0) {
        return NULL;
    }

    // prep structures and data for packet trains
    struct sockaddr_in *recv_addr = malloc(sizeof(struct sockaddr));
    memset(recv_addr, 0, sizeof(struct sockaddr));
    struct timeval low_start, low_end, high_start, high_end;
    char* payload;

    // receive low entropy packets
    uint16_t first_low_udp = 0;
    uint16_t last_low_udp = 0;
    bool begin = false;

    for (int i = 0; i < configs->udp_train_size; i++) {
        if ((payload = receive_packet(udp_sock, recv_addr)) == NULL) {
            if (errno == 11) { // EAGAIN
                LOGP("Low entropy timeout.\n");
                break;
            }
            return NULL;
        }
        // receive first low entropy packet
        if (!begin) {
            gettimeofday(&low_start, NULL);
            first_low_udp = payload[0] << 8 | payload[1];
            begin = true;
        }
        gettimeofday(&low_end, NULL);
        last_low_udp = payload[0] << 8 | payload[1];
    }

    LOG("First low udp id: %d\n", first_low_udp);
    LOG("Last low udp id: %d\n", last_low_udp);
    LOGP("First train received.\n");

    // receive high entropy packets
    uint16_t first_high_udp = 0; 
    uint16_t last_high_udp = 0;
    begin = false;
    
    for (int i = 0; i < configs->udp_train_size; i++) {
        if ((payload = receive_packet(udp_sock, recv_addr)) == NULL) {
            if (errno == 11) { // EAGAIN
                LOGP("High entropy timeout.\n");
                break;
            }
            return NULL;
        }
        // receive first high entropy packet
        if (!begin) {
            gettimeofday(&high_start, NULL);
            first_high_udp = payload[0] << 8 | payload[1];
            begin = true;
        }
        gettimeofday(&high_end, NULL);
        last_high_udp = payload[0] << 8 | payload[1];
    }

    LOG("First high udp id: %d\n", first_high_udp);
    LOG("Last high udp id: %d\n", last_high_udp);
    LOGP("Second train received.\n");

    // compression detection calculations
    char *result;
    double low_delta = time_diff_milli(low_end, low_start);
    double high_delta = time_diff_milli(high_end, high_start);
    double difference = high_delta - low_delta;

    LOG("Low entropy: %.0fms\n", low_delta);
    LOG("High entropy: %.0fms\n", high_delta);
    LOG("Delta: %.0fms\n", difference);

    if (difference > configs->threshold) {
        result = "Compression detected.";
    } else {
        result = "No compression detected.";
    }

    // free memory
    free(my_addr);
    free(recv_addr);
    free(payload);

    // close socket
    if (close(udp_sock) < 0) {
        perror("Error closing udp socket");
        return NULL;
    }

    return result;
}

/**
 * Post-probing phase of compression detection. Accepts a
 * TCP connection and sends compression status to client.
 *
 * listen_port: port to listen on
 * msg: compression results
 *
 * returns: 1 if successful, -1 otherwise
 */
int post_probing(uint16_t listen_port, char *msg)
{
    // bind port and accept client connection
    int tcp_sock;
    if ((tcp_sock = create_tcp_socket()) < 0) {
        return -1;
    }

    if (bind_and_listen(tcp_sock, listen_port) < 0) {
        return -1;
    }

    int client_sock;
    if ((client_sock = accept_connection(tcp_sock)) < 0) {
        return -1;
    }

    // send compression results
    if (send_stream(client_sock, msg) < 0) {
        return -1;
    }

    LOGP("Compression status sent, closing TCP connection.\n");
    if (close(tcp_sock) < 0) {
        perror("Error closing tcp socket");
        return -1;
    }
    if (close(client_sock) < 0) {
        perror("Error closing client socket");
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
