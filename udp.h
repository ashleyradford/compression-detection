/**
 * @file
 *
 * Contains udp communication functions
 */

#ifndef _UDP_H_
#define _UDP_H_

int create_udp_socket();
struct sockaddr_in* get_addr_in(int port);
int bind_port(int sockfd, struct sockaddr_in *addr_in);
int send_packets(int sockfd, char *msg, struct sockaddr_in *addr_in);
char* receive_packets(int sockfd, struct sockaddr_in *addr_in);

#endif
