/**
 * @file
 *
 * Contains tcp communication functions
 */

#ifndef _TCP_H_
#define _TCP_H_

#include <netinet/in.h>

int create_tcp_socket();
int establish_connection(int sockfd, in_addr_t server_ip, unsigned short server_port);
int bind_and_listen(int sockfd, unsigned short port);
int accept_connection(int sockfd);
int send_stream(int sockfd, char *msg);
char* receive_stream(int sockfd);

#endif
