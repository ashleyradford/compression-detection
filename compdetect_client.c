#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/stat.h>
#include <arpa/inet.h>

#include "cJSON.h"
#include "tcp.h"

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

int main(int argc, char *argv[])
{
    // check that config file is provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s config_file.json\n", argv[0]);
        return EXIT_FAILURE;
    }

    // --- PRE PROBING PHASE ---
    char *filename = argv[1];
    struct stat buf;
    if (stat(filename, &buf) < 0) {
        perror("Error retrieving file info");
        return EXIT_FAILURE;
    };

    // read in config file contents
    char *config_contents = read_file(argv[1],  buf.st_size);

    // parse config file
    struct client_config *configs = malloc(sizeof(struct client_config));
    parse_config(configs, config_contents);

    // establish connection and send config contents
    int sockfd;
    if ((sockfd = create_socket()) < 0) {
        return EXIT_FAILURE;
    }

    if ((sockfd = establish_connection(sockfd, configs->server_ip, configs->tcp_port)) < 0) {
        return EXIT_FAILURE;
    }

    if (send_msg(sockfd, config_contents) < 0) {
        return EXIT_FAILURE;
    }
    
    close(sockfd);
    printf("Config contents sent, TCP connection closed.\n");

    // --- PROBING PHASE ---
    sleep(5);

    // --- POST PROBING PHASE ---
    if ((sockfd = create_socket()) < 0) {
        return EXIT_FAILURE;
    }

    if ((sockfd = establish_connection(sockfd, configs->server_ip, configs->tcp_port)) < 0) {
        return EXIT_FAILURE;
    }
    
    char *msg;
    if ((msg = receive_msg(sockfd)) == NULL) {
        return EXIT_FAILURE;
    }

    printf("Received msg: %s", msg);

    close(sockfd);

    // free memory
    free(config_contents);
    free(configs);

    return EXIT_SUCCESS;
}
