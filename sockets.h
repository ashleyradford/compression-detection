/**
 * @file
 *
 * Contains tcp, udp, and raw socket functions
 */

#ifndef _SOCKETS_H_
#define _SOCKETS_H_

#include <stdint.h>

#define RECV_BUFFER 1024

struct sockaddr_in* set_addr_struct(char* ip, uint16_t port);
int create_raw_socket();
int add_timeout(int sockfd, int wait_time);
int set_df_bit(int sockfd);
int create_tcp_socket();
int establish_connection(int sockfd, char* server_ip, uint16_t server_port);
int bind_and_listen(int sockfd, uint16_t port);
int accept_connection(int sockfd);
int send_stream(int sockfd, char *msg);
char* receive_stream(int sockfd);
int create_udp_socket();
int bind_port(int sockfd, struct sockaddr_in *addr_in);
int send_packet(int sockfd, char *packet, int packet_size, struct sockaddr_in *addr_in);
char* receive_packet(int sockfd, struct sockaddr_in *addr_in);

#endif
