/**
 * @file
 *
 * Client application that detects compression in a cooperative environment.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <netinet/in.h>

#include "cJSON.h"
#include "sockets.h"
#include "util.h"
#include "logger.h"

struct client_config {
    char* server_ip;
    uint16_t udp_source_port;
    uint16_t udp_dest_port;
    uint16_t tcp_port;
    int udp_payload_size;
    int inter_measurement_time;
    int udp_train_size;
    int udp_timeout;
    int udp_ttl;
};

/**
 * Parses JSON file for client specific configurations
 *
 * configs: client_config struct to fill
 * contents: json text to parse
 */
void parse_config(struct client_config *configs, char *contents)
{
    cJSON *root = cJSON_Parse(contents);
    configs->server_ip = cJSON_GetObjectItem(root, "server_ip")->valuestring;
    configs->udp_dest_port = atoi(cJSON_GetObjectItem(root, "udp_dest_port")->valuestring);
    configs->udp_source_port = atoi(cJSON_GetObjectItem(root, "udp_source_port")->valuestring);
    configs->tcp_port = atoi(cJSON_GetObjectItem(root, "tcp_port")->valuestring);
    configs->udp_payload_size = atoi(cJSON_GetObjectItem(root, "udp_payload_size")->valuestring);
    configs->inter_measurement_time = atoi(cJSON_GetObjectItem(root, "inter_measurement_time")->valuestring);
    configs->udp_train_size = atoi(cJSON_GetObjectItem(root, "udp_train_size")->valuestring);
    configs->udp_timeout = atoi(cJSON_GetObjectItem(root, "udp_timeout")->valuestring);
    configs->udp_ttl = atoi(cJSON_GetObjectItem(root, "udp_ttl")->valuestring);
}

/**
 * Pre-probing phase of compression detection. Establishes a
 * TCP connection and sends over file contents.
 *
 * filename: file to read, parse, and send
 *
 * returns: client_config struct parsed from filename
 */
struct client_config* pre_probing(char *filename)
{
    struct stat buf;
    if (stat(filename, &buf) < 0) {
        perror("Error retrieving file info");
        return NULL;
    }

    // read in config file contents
    char *config_contents = read_file(filename, buf.st_size);
    if (config_contents == NULL) {
        return NULL;
    }

    // parse config file
    struct client_config *configs = malloc(sizeof(struct client_config));
    parse_config(configs, config_contents);

    // create socket and establish connection
    int tcp_sock;
    if ((tcp_sock = create_tcp_socket()) < 0) {
        return NULL;
    }
    if ((tcp_sock = establish_connection(tcp_sock, configs->server_ip, configs->tcp_port)) < 0) {
        return NULL;
    }

    // send config data
    if (send_stream(tcp_sock, config_contents) < 0) {
        return NULL;
    }
    
    LOGP("Config contents sent, closing TCP connection.\n");
    if (close(tcp_sock) < 0) {
        perror("Error closing tcp socket");
        return NULL;
    }
    free(config_contents);

    return configs;
}

/**
 * Probing phase of compression detection. Sends two sets of
 * UDP packets back to back, one with low entropy and one with
 * high entropy.
 *
 * configs: pointer to client_config struct
 *
 * returns: 1 if successful, -1 otherwise
 */
int probing(struct client_config *configs)
{
    int udp_sock;
    if ((udp_sock = create_udp_socket()) < 0) {
        return -1;
    }

    // set DF bit
    if (set_df_opt(udp_sock) < 0) {
        return -1;
    }
    // add TTL opt
    if (add_ttl_opt(udp_sock, configs->udp_ttl) < 0) {
        return -1;
    }
    // bind to specified port
    struct sockaddr_in *my_addr_udp = set_addr_struct(INADDR_ANY, configs->udp_source_port);
    if (bind_port(udp_sock, my_addr_udp) < 0) {
        return -1;
    }

    // set up addr struct
    struct sockaddr_in *serv_addr; 
    if ((serv_addr = set_addr_struct(configs->server_ip, configs->udp_dest_port)) == NULL) {
        return -1;
    }

    // low entropy train
    char *payload;
    for (int i = 0; i < configs->udp_train_size; i++) {
        payload = create_low_entropy_payload(i, configs->udp_payload_size);
        if (payload == NULL) {
            return -1;
        }
        // send low entropy packets
        if (send_packet(udp_sock, payload, configs->udp_payload_size, serv_addr) < 0) {
            return -1;
        }
    }

    LOG("First train sent. Sleeping for %ds.\n", configs->inter_measurement_time);
    sleep(configs->inter_measurement_time);

    // high entropy train
    for (int i = 0; i < configs->udp_train_size; i++) {
        payload = create_high_entropy_payload(i, configs->udp_payload_size);
        if (payload == NULL) {
            return -1;
        }
        // send high entropy packets
        if (send_packet(udp_sock, payload, configs->udp_payload_size, serv_addr) < 0) {
            return -1;
        }
    }

    LOGP("Second train sent.\n");
    free(payload);

    // close socket
    if (close(udp_sock) < 0) {
        perror("Error closing udp socket");
        return -1;
    }

    return 1;
}

/**
 * Post-probing phase of compression detection. Establishes a
 * TCP connection and receives compression status from server.
 *
 * configs: pointer to client_config struct
 *
 * returns: 1 if successful, -1 otherwise
 */
int post_probing(struct client_config *configs)
{
    // create socket and establish connection
    int tcp_sock;
    if ((tcp_sock = create_tcp_socket()) < 0) {
        return -1;
    }

    if ((tcp_sock = establish_connection(tcp_sock, configs->server_ip, configs->tcp_port)) < 0) {
        return -1;
    }

    // receive compression detection results
    char *msg;
    if ((msg = receive_stream(tcp_sock)) == NULL) {
        return -1;
    }
    printf("%s\n", msg);
    free(msg);

    // close socket
    if (close(tcp_sock) < 0) {
        perror("Error closing socket");
        return -1;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    // check that config file is provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s config_file.json\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *filename = argv[1];

    // ---- pre probing phase ----
    struct client_config *configs;
    if ((configs = pre_probing(filename)) == NULL) {
        return EXIT_FAILURE;
    }

    // ---- probing phase ----
    sleep(1); // ready for UDP
    if (probing(configs) < 0 ) {
        return EXIT_FAILURE;
    }
    // ensure server opens TCP
    sleep(configs->udp_timeout + 1);

    // ---- post probing phase ----
    if (post_probing(configs) < 0) {
        return EXIT_FAILURE;
    }

    // free config structure data
    free(configs);

    return EXIT_SUCCESS;
}
