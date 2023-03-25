#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/stat.h>
#include <arpa/inet.h>

#include "cJSON.h"
#include "tcp.h"
#include "udp.h"

struct client_config {
    in_addr_t server_ip;
    unsigned short udp_dest_port;
    unsigned short tcp_port;
    int udp_payload_size;
    int inter_measurement_time;
    int udp_train_size;
    int ttl;
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
}

char* create_payload(struct client_config *configs)
{
    char *payload = malloc(configs->udp_payload_size);
    if (payload == NULL) {
        perror("Error mallocing payload");
        return NULL;
    }
    memset(payload, 0, configs->udp_payload_size);
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

    // parse config file
    struct client_config *configs = malloc(sizeof(struct client_config));
    parse_config(configs, config_contents);

    // establish connection and send config contents
    int sockfd;
    if ((sockfd = create_tcp_socket()) < 0) {
        return NULL;
    }

    if ((sockfd = establish_connection(sockfd, configs->server_ip, configs->tcp_port)) < 0) {
        return NULL;
    }

    if (send_stream(sockfd, config_contents) < 0) {
        return NULL;
    }
    
    close(sockfd);
    printf("Config contents sent, TCP connection closed.\n");
    free(config_contents);

    return configs;
}

int probing(struct client_config *configs)
{
    int sock;
    if ((sock = create_udp_socket()) < 0) {
        return -1;
    }

    struct sockaddr_in *my_addr; 
    if ((my_addr = get_addr_in(configs->udp_dest_port)) == NULL) {
        return -1;
    }

    char* payload = create_payload(configs);
    if (payload == NULL) {
        return -1;
    }
    printf("Payload: %s\n", payload);

    if (send_packet(sock, payload, my_addr) < 0) {
        return -1;
    }
    
    // use a short int
    // incremenet using bit wise op
    // move most sig and lest sig

    // using clock
    // get time of day (better one to use)

    return 1;
}

int post_probing(struct client_config *configs)
{
    int sockfd;
    if ((sockfd = create_tcp_socket()) < 0) {
        return -1;
    }

    if ((sockfd = establish_connection(sockfd, configs->server_ip, configs->tcp_port)) < 0) {
        return -1;
    }
    
    char *msg;
    if ((msg = receive_stream(sockfd)) == NULL) {
        return -1;
    }
    printf("Received msg: %s", msg);

    close(sockfd);

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

    // pre probing phase
    struct client_config *configs;
    if ((configs = pre_probing(filename)) == NULL) {
        return EXIT_FAILURE;
    }
    
    // probing phase
    sleep(5);
    probing(configs);

    // post probing phase
    // if (post_probing(configs) < 0) {
    //     return EXIT_FAILURE;
    // }
    
    // free config structure data
    free(configs);

    return EXIT_SUCCESS;
}
