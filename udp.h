/**
 * @file
 *
 * Contains udp communication functions
 */

#ifndef _UDP_H_
#define _UDP_H_

int create_udp_socket();
int add_timeout(int sockfd, int wait_time);
int set_df(int sockfd);
struct sockaddr_in* get_addr_in(in_addr_t ip, int port);
int bind_port(int sockfd, struct sockaddr_in *addr_in);
int send_packet(int sockfd, char *payload, int payload_size, struct sockaddr_in *addr_in);
char* receive_packet(int sockfd, struct sockaddr_in *addr_in);

#endif
