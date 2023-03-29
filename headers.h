/**
 * @file
 *
 * Defines ip and tcp header helper functions.
 */

#ifndef _HEADERS_H_
#define _HEADERS_H_

#define IP4_HDRLEN 20
#define TCP_HDRLEN 20

char* create_syn_packet(struct sockaddr_in *src_addr, struct sockaddr_in *dst_addr, int len);

#endif
