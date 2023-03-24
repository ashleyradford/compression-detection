/**
 * @file
 *
 * Contains tcp config structures and communication functions
 */

#ifndef _TCP_H_
#define _TCP_H_

#include <netinet/in.h>

int create_socket();
int establish_connection(int sockfd, in_addr_t server_ip, unsigned short server_port);
int bind_and_listen(int sockfd, unsigned short port);
int accept_connection(int sockfd);
int send_msg(int sockfd, char *msg);
char* receive_msg(int sockfd);

#endif
