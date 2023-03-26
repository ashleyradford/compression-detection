#include <netinet/ip.h>
#include <netinet/tcp.h>

#define IP4_HDRLEN 20
#define TCP_HDRLEN 20

int fill_in_iphdr(struct ip *iphdr)
{



}

int fill_in_tcphdr(struct tcphdr *tcphdr)
{



}


int create_raw_socket()
{
    struct ip *iphdr;
    struct tcphdr *tcphdr;

    // malloc memory for headers
    iphdr = malloc(sizeof(struct ip));
    if (iphdr == NULL) {
        perror("Error mallocing memory for ip header");
        return EXIT_FAILURE;
    }

    tcphdr = malloc(sizeof(struct tcphdr));
    if (tcphdr == NULL) {
        perror("Effor mallocing memory for tcp header");
        return EXIT_FAILURE;
    }

    // set headers
    if (set_iphdr(iphdr) < 0) {
        return EXIT_FAILURE;
    }

    if (set_tcphdr(tcphdr) < 0) {
        return EXIT_FAILURE;
    }
}

