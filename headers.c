#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <netinet/ip.h>
#include <netinet/tcp.h>

#include "logger.h"

#define IP4_HDRLEN 20
#define TCP_HDRLEN 20

struct pseudo_header {
    uint32_t source_address;
    uint32_t dest_address;
    uint8_t reserved;
    uint8_t protocol;
    uint16_t tcp_length;
};

// source: https://github.com/MaxXor/raw-sockets-example
uint16_t checksum(const char *buf, uint32_t size)
{
    uint32_t sum = 0, i;

    // accumulate checksum
    for (i = 0; i < size - 1; i += 2) {
        uint16_t word16 = *(uint16_t *) &buf[i];
        sum += word16;
    }

    // handle odd-sized case
    if (size & 1) {
        uint16_t word16 = (unsigned char) buf[i];a
        sum += word16;
    }

    // fold to get the ones-complement result
    while (sum >> 16) sum = (sum & 0xFFFF)+(sum >> 16);

    // invert to get the negative in ones-complement arithmetic
    return ~sum;
}

void fill_in_iphdr(struct ip* iphdr, struct sockaddr_in *src_addr, struct sockaddr_in *dst_addr)
{
    iphdr->ip_v = 4;                                 // version (4 bits)
    iphdr->ip_hl = IP4_HDRLEN / sizeof(uint32_t);    // header length (4 bits)
    iphdr->ip_tos = 0;                               // type of service (8 bits)
    iphdr->ip_len = htons(IP4_HDRLEN + TCP_HDRLEN);  // total length (16 bits)
    iphdr->ip_id = htons(0);                         // id (16 bits)
    
    // flags and fragmentation
    int ip_flags[4] = {0, 0, 0, 0};
    ip_flags[0] = 0;    // zero bit
    ip_flags[1] = 0;    // don't fragment bit
    ip_flags[2] = 0;    // more fragment bit
    ip_flags[3] = 0;    // fragment offset
    iphdr->ip_off = htons((ip_flags[0] << 15)
                        + (ip_flags[1] << 14)
                        + (ip_flags[2] << 13)
                        +  ip_flags[3]);

    iphdr->ip_ttl = 255;        // time to live (8 bits)
    iphdr->ip_p = IPPROTO_TCP;  // protocol (8 bits)
    iphdr->ip_sum = 0;          // header checksum, first set to 0 (16 bits)
    
    iphdr->ip_src = src_addr->sin_addr;  // source IPv4 address (32 bits)
    iphdr->ip_dst = dst_addr->sin_addr;  // destination IPv4 address (32 bits)

    LOGP("Successfully filled in IP header (with 0 checksum).\n");
}

void fill_in_tcphdr(struct tcphdr* tcphdr, struct sockaddr_in *src_addr, struct sockaddr_in *dst_addr)
{
    tcphdr->th_sport = src_addr->sin_port;          // source port (16 bits)
    tcphdr->th_dport = dst_addr->sin_port;          // dest port (16 bits)
    tcphdr->th_seq = htonl(rand() % 4294967295);    // seq number (32 bits)
    tcphdr->th_ack = htonl(0);                      // ack number (32 bits)
    tcphdr->th_x2 = 0;                              // reserved (4 bits)
    tcphdr->th_off = TCP_HDRLEN / sizeof(uint32_t); // data offset (4 bits)

    // flags (will be 8 bits)
    int tcp_flags[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    tcp_flags[0] = 0;   // FIN flag (1 bit)
    tcp_flags[1] = 1;   // SYN flag (1 bit)
    tcp_flags[2] = 0;   // RST flag (1 bit)
    tcp_flags[3] = 0;   // PSH flag (1 bit)
    tcp_flags[4] = 0;   // ACK flag (1 bit)
    tcp_flags[5] = 0;   // URG flag (1 bit)
    tcp_flags[6] = 0;   // URG flag (1 bit)
    tcp_flags[7] = 0;   // CWR flag (1 bit)

    tcphdr->th_flags = 0;
    for (int i = 0; i < 8; i++) {
        tcphdr->th_flags += (tcp_flags[i] << i);
    }

    tcphdr->th_win = htons(65535);  // window size (16 bits)
    tcphdr->th_sum = 0;             // header checksum, first set to 0 (16 bits)
    tcphdr->th_urp = htons(0);      // urgent pointer (16 bits)    

    LOGP("Successfully filled in TCP header (with 0 checksum).\n");
}

// Create a pseudo-header by combining the source IP address, destination
// IP address, protocol number (6 for TCP), and the TCP segment length. 
// The TCP segment length is the length of the TCP header and data in bytes.
// We have no data, so for us it will jsut be the TCP header.
char* create_syn_packet(struct sockaddr_in *src_addr, struct sockaddr_in *dst_addr, int len)
{
    // datagram to represent the packet
    char *datagram = malloc(len);
    if (datagram == NULL) {
        perror("Error mallocing syn packet");
        return NULL;
    }
    memset(datagram, 0, len);

    // required structs for IP and TCP header
    struct ip *iphdr = (struct ip*) datagram;
    struct tcphdr *tcphdr = (struct tcphdr*) (datagram + sizeof(struct ip));

    // fill in headers
    fill_in_iphdr(iphdr, src_addr, dst_addr);
    fill_in_tcphdr(tcphdr, src_addr, dst_addr);

    struct pseudo_header psh;
    psh.source_address = src_addr->sin_addr.s_addr;
    psh.dest_address = dst_addr->sin_addr.s_addr;
    psh.reserved = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr));

    // pseudo header + tcp header + data = checksum
    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr);
    
    // fill pseudo packet
    char* pseudogram = malloc(psize);
    if (pseudogram == NULL) {
        perror("Error mallocing pseudogram");
        return NULL;
    }
    memcpy(pseudogram, (char*) &psh, sizeof(struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header), tcphdr, sizeof(struct tcphdr));

    // calculate and store checksums
    // tcphdr->th_sum = checksum((const char*) pseudogram, psize);
    // iphdr->ip_sum = checksum((const char*) datagram, iphdr->ip_len);

    LOG("TCP checksum: %d\n", tcphdr->th_sum);
    LOG("IP checksum: %d\n", iphdr->ip_sum);

    free(pseudogram);

    LOGP("Successfully generated checksums.\n");
    return datagram;
}
