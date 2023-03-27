#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "cJSON.h"
#include "headers.h"
#include "logger.h"

#define RECV_BUFFER 1024

struct config {
    char* server_ip;
    uint16_t tcp_head_dest;
    uint16_t tcp_tail_dest;
    uint16_t udp_dest_port;
    int udp_payload_size;
    int inter_measurement_time;
    int udp_train_size;
    int udp_timeout;
};

void parse_config(struct config *configs, char *contents)
{
    // parse json
    cJSON *root = cJSON_Parse(contents);
    configs->server_ip = cJSON_GetObjectItem(root, "server_ip")->valuestring;
    configs->tcp_head_dest = atoi(cJSON_GetObjectItem(root, "tcp_head_dest")->valuestring);
    configs->tcp_tail_dest = atoi(cJSON_GetObjectItem(root, "tcp_tail_dest")->valuestring);
    configs->udp_dest_port = atoi(cJSON_GetObjectItem(root, "udp_dest_port")->valuestring);
    configs->udp_payload_size = atoi(cJSON_GetObjectItem(root, "udp_payload_size")->valuestring);
    configs->inter_measurement_time = atoi(cJSON_GetObjectItem(root, "inter_measurement_time")->valuestring);
    configs->udp_train_size = atoi(cJSON_GetObjectItem(root, "udp_train_size")->valuestring);
    configs->udp_timeout = atoi(cJSON_GetObjectItem(root, "udp_timeout")->valuestring);
}

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

int create_raw_socket()
{
    int sockfd;
    if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP)) < 0) {
        perror("Error creating raw socket");
        return -1;
    }

    // for the address already in use error
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        perror("Cannot reuse port");
        return -1;
    }

    return sockfd;
}

int set_ip_flag(int sockfd)
{
    const int on = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
        perror ("Cannot set IP_HDRINCL option");
        return -1;
    }

    return sockfd;
}

int send_packet(int sockfd, char *packet, int packet_size, struct sockaddr_in *addr_in)
{
    int to_len, bytes_sent;
    to_len = sizeof(struct sockaddr);
    
    if ((bytes_sent = sendto(sockfd, packet, packet_size, 0,
                        (struct sockaddr *) addr_in, to_len)) < 0) {
        perror("Error sending packet");
        return -1;
    }
    LOG("Packet sent to %d.\n", ntohs(addr_in->sin_port));
    
    return 1;
}

char* receive_packet(int sockfd, struct sockaddr_in *addr_in)
{
    char *buf = malloc(RECV_BUFFER);
    if (buf == NULL) {
        perror("Error mallocing buf");
        return NULL;
    }
    memset(buf, '\0', RECV_BUFFER);

    int bytes_received;
    uint from_len = sizeof(struct sockaddr);
    if ((bytes_received = recvfrom(sockfd, buf, RECV_BUFFER, 0,
                            (struct sockaddr *) addr_in, &from_len)) < 0) {
        if (errno == 11) {
            return NULL;
        }
        perror("Error receiving packet");
        return NULL;
    }
    LOG("Bytes recieved: %d\n", bytes_received);

    return buf;
}

struct sockaddr_in* get_addr_in(char* ip, uint16_t port)
{
    // setting up addr struct
    struct sockaddr_in *sin = malloc(sizeof(struct sockaddr_in));
    if (sin == NULL) {
        perror("Error mallocing sockaddr_in");
        return NULL;
    }
    memset(sin, 0, sizeof(struct sockaddr_in));

    sin->sin_family = AF_INET;
    sin->sin_port = htons(port);

    if (ip == NULL) {
        sin->sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, ip, &(sin->sin_addr)) != 1) {
            perror("Error converting IP address");
            return NULL;
        }
    }

    LOGP("Created socket address struct.\n");
    return sin;
}

void print_packet(char* packet)
{
    for (int i = 0; i < 40; ++i) {
        // print each byte as bits
        for (int j = 7; 0 <= j; j--) {
            printf("%c", (packet[i] & (1 << j)) ? '1' : '0');
        }   
        // print spaces
        if ((i+1) % 4 == 0) {
            printf("\n");
        } else {
            printf(" ");
        }
    }
}

int main(int argc, char *argv[])
{
    // check that config file is provided
    if (argc < 2) {
        fprintf(stderr, "Usage: %s config_file.json\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *filename = argv[1];

    struct stat buf;
    if (stat(filename, &buf) < 0) {
        perror("Error retrieving file info");
        return EXIT_FAILURE;
    }

    // read in config file contents
    char *config_contents = read_file(filename, buf.st_size);
    if (config_contents == NULL) {
        return EXIT_FAILURE;
    }

    // parse config file
    struct config *configs = malloc(sizeof(struct config));
    parse_config(configs, config_contents);

    // set up source addr struct
    struct sockaddr_in *my_addr;
    if ((my_addr = get_addr_in(0, 9000)) == NULL) {
        return EXIT_FAILURE;
    }
    // set up server addr struct
    struct sockaddr_in *serv_addr;
    if ((serv_addr = get_addr_in(configs->server_ip, configs->tcp_head_dest)) == NULL) {
        return EXIT_FAILURE;
    }

    // create raw socket
    int sock;
    if ((sock = create_raw_socket()) < 0) {
        return EXIT_FAILURE;
    }

    // set flag so socket expects our IPv4 header
    if (set_ip_flag(sock) < 0) {
        return EXIT_FAILURE;
    }

    // create syn packet
    char* syn_packet = create_syn_packet(my_addr, serv_addr, IP4_HDRLEN + TCP_HDRLEN);
    if (syn_packet == NULL) {
        perror("Error creating syn packet");
        return EXIT_FAILURE;
    }

    // print packet
    print_packet(syn_packet);

    // send fake SYN packet
    send_packet(sock, syn_packet, IP4_HDRLEN + TCP_HDRLEN, serv_addr);

    // receive RST packet
    // receive_packet(sock, my_addr);

    // close socket
    if (close(sock) < 0) {
        perror("Error closing socket");
        return EXIT_FAILURE;
    }

    // send head syn

    // receive tail syn

    // send low entropy train

    // send high entropy train

    // compare results to detect compression

    return EXIT_SUCCESS;
}
