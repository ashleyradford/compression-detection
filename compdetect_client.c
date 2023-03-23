#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "cJSON.h"

struct config_data {
    in_addr_t server_ip;
    unsigned short udp_dest_port;
    unsigned short tcp_port;
    int udp_payload_size;
    int inter_measurement_time;
    int udp_train_size;
    int ttl;
};

char* read_config(char *filename, int size)
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
        perror("Error when opening config file");
        free(buf);
        return NULL;
    }

    // read config
    if (fread(buf, size, 1, fp) < 0) {
        perror("Error when reading config");
        free(buf);
        return NULL;
    }

    return buf;
}

int parse_config(struct config_data *configs, char *contents)
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
    
    return 1;
}

int send_msg(char *msg, in_addr_t server_ip, unsigned short server_port)
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
    fprintf(stderr, "Message sent: %s", msg);
    int len, bytes_sent;
    len = strlen(msg);
    bytes_sent = send(sockfd, msg, len, 0);

    return 1;
}

int main(int argc, char *argv[])
{
    // check that config file is provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s config_file.json\n", argv[0]);
        return EXIT_FAILURE;
    }

    // get file size
    char *filename = argv[1];
    struct stat buf;
    if (stat(filename, &buf) < 0) {
        perror("Error retrieving file info");
        return 1;
    };

    // read in config file contents
    char *config_contents = read_config(argv[1],  buf.st_size);

    // parse config file
    struct config_data *configs = malloc(sizeof(struct config_data));
    if (parse_config(configs, config_contents) < 0) {
         perror("Error when parsing config");
         return -1;
    }
   
    // pre probing phase
    if (send_msg(config_contents, configs->server_ip, configs->tcp_port) < 0) {
        return EXIT_FAILURE;
    }

    // probing phase


    // post probing phase


    // free memory
    free(config_contents);
    free(configs);

    return EXIT_SUCCESS;
}
