#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/stat.h>
#include <arpa/inet.h>

#include "cJSON.h"
#include "tcp.h"
#include "udp.h"
#include "logger.h"

struct client_config {
    in_addr_t server_ip;
    uint16_t udp_dest_port;
    uint16_t tcp_port;
    int udp_payload_size;
    int inter_measurement_time;
    uint16_t udp_train_size;
    int ttl;
    int timeout;
};

char* read_file(char *filename, int size)
{
    // malloc buffer size
    char *buf = malloc(size);
    if (buf == NULL) {
        perror("Error when mallocing buf");
        return NULL;
    }

    // open file
    FILE *fp;
    if ((fp = fopen(filename, "r")) < 0) {
        perror("Error when opening file");
        free(buf);
        return NULL;
    }

    // read file
    if (fread(buf, size, 1, fp) < 0) {
        perror("Error when reading file");
        free(buf);
        return NULL;
    }
    fclose(fp);

    return buf;
}

void parse_config(struct client_config *configs, char *contents)
{
    // parse json
    cJSON *root = cJSON_Parse(contents);
    configs->server_ip = inet_addr(cJSON_GetObjectItem(root, "server_ip")->valuestring);
    configs->udp_dest_port = atoi(cJSON_GetObjectItem(root, "udp_dest_port")->valuestring);
    configs->tcp_port = atoi(cJSON_GetObjectItem(root, "tcp_port")->valuestring);
    configs->udp_payload_size = atoi(cJSON_GetObjectItem(root, "udp_payload_size")->valuestring);
    configs->inter_measurement_time = atoi(cJSON_GetObjectItem(root, "inter_measurement_time")->valuestring);
    configs->udp_train_size = atoi(cJSON_GetObjectItem(root, "udp_train_size")->valuestring);
    configs->ttl = atoi(cJSON_GetObjectItem(root, "ttl")->valuestring);
    configs->timeout = atoi(cJSON_GetObjectItem(root, "timeout")->valuestring);
}

void set_packet_id(char *payload, int id)
{
    if (id <= 255) {
        memset(payload, 0, 1);
        memset(payload + 1, id, 1);
    } else {
        memset(payload + 1, id & 255, 1);
        id >>= 8;
        memset(payload, id & 255, 1);
    }
}

char* create_low_entropy_payload(int id, int payload_size)
{
    char *payload = malloc(payload_size);
    if (payload == NULL) {
        perror("Error mallocing payload");
        return NULL;
    }
    memset(payload, 0, payload_size);

    // set packet id
    set_packet_id(payload, id);

    return payload;
}

char* create_high_entropy_payload(int id, int payload_size)
{
    char *payload = read_file("myrandom", payload_size);
    if (payload == NULL) {
        return NULL;
    }

    // set packet id
    set_packet_id(payload, id);

    return payload;
}

struct client_config* pre_probing(char *filename)
{
    struct stat buf;
    if (stat(filename, &buf) < 0) {
        perror("Error retrieving file info");
        return NULL;
    }

    // read in config file contents
    char *config_contents = read_file(filename,  buf.st_size);
    if (config_contents == NULL) {
        return NULL;
    }

    // parse config file
    struct client_config *configs = malloc(sizeof(struct client_config));
    parse_config(configs, config_contents);

    // create socket and establish connection
    int sockfd;
    if ((sockfd = create_tcp_socket()) < 0) {
        return NULL;
    }

    if ((sockfd = establish_connection(sockfd, configs->server_ip, configs->tcp_port)) < 0) {
        return NULL;
    }

    // send config data
    if (send_stream(sockfd, config_contents) < 0) {
        return NULL;
    }
    
    LOGP("Config contents sent, closing TCP connection.\n");
    if (close(sockfd) < 0) {
        perror("Error closing socket");
        return NULL;
    }
    free(config_contents);

    return configs;
}

int probing(struct client_config *configs)
{
    int sock;
    if ((sock = create_udp_socket()) < 0) {
        return -1;
    }

    // set DF bit
    if (set_df_bit(sock) < 0) {
        return -1;
    }

    // set up addr struct
    struct sockaddr_in *my_addr; 
    if ((my_addr = get_addr_in(configs->server_ip, configs->udp_dest_port)) == NULL) {
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
        if (send_packet(sock, payload, configs->udp_payload_size, my_addr) < 0) {
            return -1;
        }
    }

    LOG("First train sent.\nSleeping for %ds.\n", configs->inter_measurement_time);
    sleep(configs->inter_measurement_time);

    // high entropy train
    for (int i = 0; i < configs->udp_train_size; i++) {
        payload = create_high_entropy_payload(i, configs->udp_payload_size);
        if (payload == NULL) {
            return -1;
        }
        // send high entropy packets
        if (send_packet(sock, payload, configs->udp_payload_size, my_addr) < 0) {
            return -1;
        }
    }

    LOGP("Second train sent.\n");
    free(payload);

    return 1;
}

int post_probing(struct client_config *configs)
{
    // create socket and establish connection
    int sockfd;
    if ((sockfd = create_tcp_socket()) < 0) {
        return -1;
    }

    if ((sockfd = establish_connection(sockfd, configs->server_ip, configs->tcp_port)) < 0) {
        return -1;
    }

    // receive compression detection results
    char *msg;
    if ((msg = receive_stream(sockfd)) == NULL) {
        return -1;
    }
    printf("%s\n", msg);
    free(msg);

    // close socket
    if (close(sockfd) < 0) {
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
    sleep(configs->timeout + 1);

    // ---- post probing phase ----
    if (post_probing(configs) < 0) {
        return EXIT_FAILURE;
    }

    // free config structure data
    free(configs);

    return EXIT_SUCCESS;
}
