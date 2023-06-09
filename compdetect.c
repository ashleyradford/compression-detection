/**
 * @file
 *
 * A standalone application that detects network compression, without
 * requiring any cooperation from the server.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include "cJSON.h"
#include "headers.h"
#include "sockets.h"
#include "util.h"
#include "logger.h"

#define RECV_BUFFER 1024

struct config {
    char* client_ip;
    char* server_ip;
    uint16_t tcp_head_dest;
    uint16_t tcp_tail_dest;
    uint16_t udp_source_port;
    uint16_t udp_dest_port;
    int tcp_port;
    int udp_payload_size;
    int inter_measurement_time;
    int udp_train_size;
    int udp_timeout;
    int rst_timeout;
    int udp_ttl;
    int threshold;
};

struct thread_data {
    int sockfd;
    int rst_timeout;
    int threshold;
    struct sockaddr_in *recv_addr;
    char* result;
};

/**
 * Parses JSON file for standalone specific configurations
 *
 * configs: config struct to fill
 * contents: json text to parse
 */
void parse_config(struct config *configs, char *contents)
{
    cJSON *root = cJSON_Parse(contents);
    configs->client_ip = cJSON_GetObjectItem(root, "client_ip")->valuestring;
    configs->server_ip = cJSON_GetObjectItem(root, "server_ip")->valuestring;
    configs->tcp_port = atoi(cJSON_GetObjectItem(root, "tcp_port")->valuestring);
    configs->tcp_head_dest = atoi(cJSON_GetObjectItem(root, "tcp_head_dest")->valuestring);
    configs->tcp_tail_dest = atoi(cJSON_GetObjectItem(root, "tcp_tail_dest")->valuestring);
    configs->udp_source_port = atoi(cJSON_GetObjectItem(root, "udp_source_port")->valuestring);
    configs->udp_dest_port = atoi(cJSON_GetObjectItem(root, "udp_dest_port")->valuestring);
    configs->udp_payload_size = atoi(cJSON_GetObjectItem(root, "udp_payload_size")->valuestring);
    configs->inter_measurement_time = atoi(cJSON_GetObjectItem(root, "inter_measurement_time")->valuestring);
    configs->udp_train_size = atoi(cJSON_GetObjectItem(root, "udp_train_size")->valuestring);
    configs->udp_timeout = atoi(cJSON_GetObjectItem(root, "udp_timeout")->valuestring);
    configs->rst_timeout = atoi(cJSON_GetObjectItem(root, "rst_timeout")->valuestring);
    configs->udp_ttl = atoi(cJSON_GetObjectItem(root, "udp_ttl")->valuestring);
    configs->threshold = atoi(cJSON_GetObjectItem(root, "threshold")->valuestring);
}

/**
 * Thread process for recieving packets through a raw socket, calculates
 * compression when all 4 SYN pakcets arrive (or it times out) and
 * updates thread_data struct with the results
 *
 * arg: void pointer (preferably pointer tp thread_data struct)
 */
void* receive_routine(void *arg)
{
    // thread data to fill in or use
    struct thread_data *tdata = (struct thread_data *) arg;

    // time difference
    struct timeval low_e_head, low_e_tail, high_e_head, high_e_tail;
    bool tail = false;
    bool high_e_train = false;
    char* buf;

    struct timeval beg, curr;
    gettimeofday(&beg, NULL);
    gettimeofday(&curr, NULL);

    while(time_diff_sec(curr, beg) <= tdata->rst_timeout) {
        buf = receive_packet(tdata->sockfd, tdata->recv_addr);
        if (buf == NULL) {
            return NULL;
        }

        // check if we received RST packet
        char tcp_flags = buf[33];
        if (tcp_flags & (1 << 2)) {
            LOGP("RST packet received.\n");
            if (!tail) {
                if (!high_e_train) {
                    gettimeofday(&low_e_head, NULL);
                } else {
                    gettimeofday(&high_e_head, NULL);
                }
                tail = true;
            } else {
                if (!high_e_train) {
                    gettimeofday(&low_e_tail, NULL);
                    high_e_train = true;
                    tail = false;
                } else {
                    gettimeofday(&high_e_tail, NULL);
                    break;
                }
            }
            // reset timeout clock
            gettimeofday(&beg, NULL);
        }
        // set curr time
        gettimeofday(&curr, NULL);
    }

    // check if loop timed out
    if (time_diff_sec(curr, beg) > tdata->rst_timeout) {
        LOGP("Receive timed out.\n");
        tdata->result = "Failed to detect due to insufficient information.";
    } else {
        double low_delta = time_diff_milli(low_e_tail, low_e_head);
        double high_delta = time_diff_milli(high_e_tail, high_e_head);
        double delta = high_delta - low_delta;

        LOG("Delta result: %.0fms\n", delta);
        if (delta > tdata->threshold) {
            tdata->result = "Compression detected.";
        } else {
            tdata->result = "No compression detected.";
        }
    }

    free(buf);

    return NULL;
}

/**
 * Sends head SYN packet, low entropy train, and then tail SYN packet
 *
 * configs: pointer to config struct
 * raw_sock: raw socket file descriptor
 * head_syn_packet: pointer to head tcp syn packet
 * head_serv_addr: pointer to sockaddr_in struct for head tcp port
 * udp_sock: udp socket file descriptor
 * udp_server_addr: pointer to sockaddr_in struct for udp port
 * tail_syn_packet: pointer to tail tcp syn packet
 * tail_serv_addr: sockaddr_in struct for tail tcp port
 *
 * returns: 1 if successful, -1 otherwise
 */
int send_low_entropy_train(struct config *configs, int raw_sock, char *head_syn_packet,
                            struct sockaddr_in *head_serv_addr, int udp_sock,
                            struct sockaddr_in *udp_serv_addr, char *tail_syn_packet,
                            struct sockaddr_in *tail_serv_addr)
{
    // send head SYN packet
    // print_packet(head_syn_packet, IP4_HDRLEN + TCP_HDRLEN);
    send_packet(raw_sock, head_syn_packet, IP4_HDRLEN + TCP_HDRLEN, head_serv_addr);
    LOGP("Low entropy head syn sent.\n");

    // send low entropy UDP packet train
    char *payload;
    for (int i = 0; i < configs->udp_train_size; i++) {
        payload = create_low_entropy_payload(i, configs->udp_payload_size);
        if (payload == NULL) {
            return -1;
        }
        // send low entropy packets
        if (send_packet(udp_sock, payload, configs->udp_payload_size, udp_serv_addr) < 0) {
            return -1;
        }
    }

    LOGP("Low entropy train sent.\n");
    free(payload);

    // send tail SYN packet
    // print_packet(tail_syn_packet, IP4_HDRLEN + TCP_HDRLEN);
    send_packet(raw_sock, tail_syn_packet, IP4_HDRLEN + TCP_HDRLEN, tail_serv_addr);
    LOGP("Low entropy tail syn sent.\n");

    return 1;
}

/**
 * Sends head SYN packet, high entropy train, and then tail SYN packet
 *
 * configs: pointer to config struct
 * raw_sock: raw socket file descriptor
 * head_syn_packet: pointer to head tcp syn packet
 * head_serv_addr: pointer to sockaddr_in struct for head tcp port
 * udp_sock: udp socket file descriptor
 * udp_server_addr: pointer to sockaddr_in struct for udp port
 * tail_syn_packet: pointer to tail tcp syn packet
 * tail_serv_addr: sockaddr_in struct for tail tcp port
 *
 * returns: 1 if successful, -1 otherwise
 */
int send_high_entropy_train(struct config *configs, int raw_sock, char *head_syn_packet,
                            struct sockaddr_in *head_serv_addr, int udp_sock,
                            struct sockaddr_in *udp_serv_addr, char *tail_syn_packet,
                            struct sockaddr_in *tail_serv_addr)
{
    // send head SYN packet
    // print_packet(head_syn_packet, IP4_HDRLEN + TCP_HDRLEN);
    send_packet(raw_sock, head_syn_packet, IP4_HDRLEN + TCP_HDRLEN, head_serv_addr);
    LOGP("High entropy head syn sent.\n");

    // send high UDP packet train
    char *payload;
    for (int i = 0; i < configs->udp_train_size; i++) {
        payload = create_high_entropy_payload(i, configs->udp_payload_size);
        if (payload == NULL) {
            return -1;
        }
        // send low entropy packets
        if (send_packet(udp_sock, payload, configs->udp_payload_size, udp_serv_addr) < 0) {
            return -1;
        }
    }

    LOGP("High entropy train sent.\n");
    free(payload);

    // send tail SYN packet
    // print_packet(tail_syn_packet, IP4_HDRLEN + TCP_HDRLEN);
    send_packet(raw_sock, tail_syn_packet, IP4_HDRLEN + TCP_HDRLEN, tail_serv_addr);
    LOGP("High entropy tail syn sent.\n");

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

    free(config_contents);

    // -------- create address structs --------
    // addr struct for my tcp port
    struct sockaddr_in *my_tcp_addr;
    if ((my_tcp_addr = set_addr_struct(configs->client_ip, configs->tcp_port)) == NULL) {
        return EXIT_FAILURE;
    }

    // addr struct for my udp port
    struct sockaddr_in *my_udp_addr;
    if ((my_udp_addr = set_addr_struct(INADDR_ANY, configs->udp_source_port)) == NULL) {
        return EXIT_FAILURE;
    }

    // addr struct for server head tcp port
    struct sockaddr_in *head_serv_addr;
    if ((head_serv_addr = set_addr_struct(configs->server_ip, configs->tcp_head_dest)) == NULL) {
        return EXIT_FAILURE;
    }

    // addr struct for server udp port
    struct sockaddr_in *udp_serv_addr; 
    if ((udp_serv_addr = set_addr_struct(configs->server_ip, configs->udp_dest_port)) == NULL) {
        return EXIT_FAILURE;
    }

    // addr struct for server tail tcp port
    struct sockaddr_in *tail_serv_addr;
    if ((tail_serv_addr = set_addr_struct(configs->server_ip, configs->tcp_tail_dest)) == NULL) {
        return EXIT_FAILURE;
    }

    // -------- create sockets --------
    // create raw socket
    int raw_sock;
    if ((raw_sock = create_raw_socket()) < 0) {
        return EXIT_FAILURE;
    }

    // create udp socket
    int udp_sock;
    if ((udp_sock = create_udp_socket()) < 0) {
        return EXIT_FAILURE;
    }

    // set DF bit
    if (set_df_opt(udp_sock) < 0) {
        return EXIT_FAILURE;
    }
    // add TTL opt
    if (add_ttl_opt(udp_sock, configs->udp_ttl) < 0) {
        return EXIT_FAILURE;
    }
    // bind to specified port
    if (bind_port(udp_sock, my_udp_addr) < 0) {
        return EXIT_FAILURE;
    }

    // -------- create syn packets --------
    char* head_syn_packet = create_syn_packet(my_tcp_addr, head_serv_addr, IP4_HDRLEN + TCP_HDRLEN);
    if (head_syn_packet == NULL) {
        return EXIT_FAILURE;
    }

    char* tail_syn_packet = create_syn_packet(my_tcp_addr, tail_serv_addr, IP4_HDRLEN + TCP_HDRLEN);
    if (tail_syn_packet == NULL) {
        return EXIT_FAILURE;
    }

    // -------- start receive thread --------
    pthread_t receive_thread;

    // set up thread data
    struct thread_data *tdata = malloc(sizeof(struct thread_data));
    tdata->sockfd = raw_sock;
    tdata->rst_timeout = configs->rst_timeout;
    tdata->threshold = configs->threshold;
    struct sockaddr_in *recv_addr = malloc(sizeof(struct sockaddr));
    memset(recv_addr, 0, sizeof(struct sockaddr));
    tdata->recv_addr = recv_addr;
    
    if (pthread_create(&receive_thread, NULL, receive_routine, (void *) tdata) < 0) {
        perror("Error creating receive thread");
        return EXIT_FAILURE;
    }

    // -------- send entropy trains --------
    if (send_low_entropy_train(configs, raw_sock, head_syn_packet, head_serv_addr,
                                udp_sock, udp_serv_addr, tail_syn_packet, tail_serv_addr) < 0) {
        return EXIT_FAILURE;
    }

    LOG("Sent low entropy tail syn packet. Sleeping for %ds.\n", configs->inter_measurement_time);
    sleep(configs->inter_measurement_time);

    if (send_high_entropy_train(configs, raw_sock, head_syn_packet, head_serv_addr,
                                udp_sock, udp_serv_addr, tail_syn_packet, tail_serv_addr) < 0) {
        return EXIT_FAILURE;
    }

    // ------- join thread and receive results -------
    void* status;
    if (pthread_join(receive_thread, &status) < 0) {
        perror("Error joining thread");
        return EXIT_FAILURE;
    }

    // check thread status
    // if (status < 0) {
    //     return EXIT_FAILURE;
    // }

    // print result
    printf("%s\n", tdata->result);

    // free memory
    free(configs);
    free(head_syn_packet);
    free(tail_syn_packet);

    // free addr structs
    free(my_tcp_addr);
    free(my_udp_addr);
    free(head_serv_addr);
    free(udp_serv_addr);
    free(tail_serv_addr);

    // free thread data
    free(recv_addr);
    free(tdata);

    // close sockets
    if (close(raw_sock) < 0) {
        perror("Error closing raw socket");
        return EXIT_FAILURE;
    }
    if (close(udp_sock) < 0) {
        perror("Error closing udp socket");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
