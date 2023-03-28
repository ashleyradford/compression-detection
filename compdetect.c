#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
    uint16_t udp_dest_port;
    int tcp_port;
    int udp_payload_size;
    int inter_measurement_time;
    int udp_train_size;
    int udp_timeout;
};

struct thread_data {
    int sockfd;
    struct sockaddr_in *sin;
    double result;
};

void parse_config(struct config *configs, char *contents)
{
    // parse json
    cJSON *root = cJSON_Parse(contents);
    configs->client_ip = cJSON_GetObjectItem(root, "client_ip")->valuestring;
    configs->server_ip = cJSON_GetObjectItem(root, "server_ip")->valuestring;
    configs->tcp_port = atoi(cJSON_GetObjectItem(root, "tcp_port")->valuestring);
    configs->tcp_head_dest = atoi(cJSON_GetObjectItem(root, "tcp_head_dest")->valuestring);
    configs->tcp_tail_dest = atoi(cJSON_GetObjectItem(root, "tcp_tail_dest")->valuestring);
    configs->udp_dest_port = atoi(cJSON_GetObjectItem(root, "udp_dest_port")->valuestring);
    configs->udp_payload_size = atoi(cJSON_GetObjectItem(root, "udp_payload_size")->valuestring);
    configs->inter_measurement_time = atoi(cJSON_GetObjectItem(root, "inter_measurement_time")->valuestring);
    configs->udp_train_size = atoi(cJSON_GetObjectItem(root, "udp_train_size")->valuestring);
    configs->udp_timeout = atoi(cJSON_GetObjectItem(root, "udp_timeout")->valuestring);
}

void* receive_routine(void *arg)
{
    // data to use and fill in
    struct thread_data *tdata = (struct thread_data *) arg;

    // time difference
    struct timeval head_syn, tail_syn;
    bool tail = false;
    char* buf;

    while(1) {
        buf = receive_packet(tdata->sockfd, tdata->sin);
        if (buf == NULL) {
            // what do here
            exit(-1);
        }

        // check if RST packet
        char tcp_flags = buf[33];
        if (tcp_flags & (1 << 2)) {
            LOGP("RST packet received.\n"); // also check port?
            if (!tail) {
                gettimeofday(&head_syn, NULL);
                tail = true;
            } else {
                gettimeofday(&tail_syn, NULL);
                break;
            }
        }        
    }

    tdata->result = time_diff(tail_syn, head_syn);
    free(buf);

    return NULL;
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
    if ((my_addr = set_addr_struct(configs->client_ip, configs->tcp_port)) == NULL) {
        return EXIT_FAILURE;
    }

    // create raw socket
    int sock;
    if ((sock = create_raw_socket()) < 0) {
        return EXIT_FAILURE;
    }

    // add timeout
    if (add_timeout(sock, configs->udp_timeout) < 0) {
        return EXIT_FAILURE;
    }

    // start receive thread
    struct thread_data *low_rst = malloc(sizeof(struct thread_data));
    low_rst->sockfd = sock;
    low_rst->sin = my_addr;
    pthread_t receive_thread;
    void* status;
    if (pthread_create(&receive_thread, NULL, receive_routine, (void *) low_rst) < 0) {
        perror("Error creating receive thread");
        return EXIT_FAILURE;
    }

    // ------ send head SYN packet ------
    // set up server addr struct
    struct sockaddr_in *head_serv_addr;
    if ((head_serv_addr = set_addr_struct(configs->server_ip, configs->tcp_head_dest)) == NULL) {
        return EXIT_FAILURE;
    }

    // create syn packet
    char* head_syn_packet = create_syn_packet(my_addr, head_serv_addr, IP4_HDRLEN + TCP_HDRLEN);
    if (head_syn_packet == NULL) {
        return EXIT_FAILURE;
    }
    // print_packet(head_syn_packet, IP4_HDRLEN + TCP_HDRLEN);
    send_packet(sock, head_syn_packet, IP4_HDRLEN + TCP_HDRLEN, head_serv_addr);

    // ------ send tail SYN packet -----
    // set up server addr struct
    struct sockaddr_in *tail_serv_addr;
    if ((tail_serv_addr = set_addr_struct(configs->server_ip, configs->tcp_tail_dest)) == NULL) {
        return EXIT_FAILURE;
    }

    // create syn packet
    char* tail_syn_packet = create_syn_packet(my_addr, tail_serv_addr, IP4_HDRLEN + TCP_HDRLEN);
    if (tail_syn_packet == NULL) {
        return EXIT_FAILURE;
    }
    send_packet(sock, tail_syn_packet, IP4_HDRLEN + TCP_HDRLEN, tail_serv_addr);

    if (pthread_join(receive_thread, &status) < 0) {
        perror("Error joining thread");
        return EXIT_FAILURE;
    }

    // // check thread status
    // if (status < 0) {
    //     return EXIT_FAILURE;
    // }

    LOG("First train delta result: %.0fms\n", low_rst->result);

    // so have main thread to send
    // but start a pthread to receive first, and check if RST
    // start clock when we get first RST and then the second RST

    // send head syn

    // send low entropy train

    // send tail syn 

    // free packet and close socket

    // sleep intermeasurment after
    
    // send head syn

    // send high entropy train

    // send tail syn

    // compare results to detect compression

    // free memory
    free(low_rst);
    free(head_syn_packet);
    free(tail_syn_packet);

    // close socket
    if (close(sock) < 0) {
        perror("Error closing socket");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
