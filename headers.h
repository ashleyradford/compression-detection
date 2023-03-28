/**
 * @file
 *
 * Contains ip and tcp header functions
 */

#ifndef _HEADERS_H_
#define _HEADERS_H_

#define IP4_HDRLEN 20
#define TCP_HDRLEN 20

void fill_in_iphdr(struct ip* iphdr, int ttl, struct sockaddr_in *src_addr, struct sockaddr_in *dst_addr);
void create_tcphdr(struct tcphdr* tcphdr, struct sockaddr_in *src_addr, struct sockaddr_in *dst_addr);
char* create_syn_packet(struct sockaddr_in *src_addr, struct sockaddr_in *dst_addr, int len, int ttl);

#endif
