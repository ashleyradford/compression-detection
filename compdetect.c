#include "logger.h"
#include <netinet/ip.h>
#include <netinet/tcp.h>

int create_raw_socket()
{
    int sockfd;
    if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP)) < 0) {
        perror("Error creating raw socket");
        return -1;
    }

    return sockfd;
}

int set_ip_flag(int sockfd)
{
    const int on = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
        perror ("Cannot set IP_HDRINCL option");
        return -1;
    }

    return sockfd;
}

int main(int argc, char *argv[])
{
    // create raw socket
    int sock;
    if ((sock = create_tcp_socket()) < 0) {
        return EXIT_FAILURE;
    }

    // set flag so socket expects our IPv4 header
    if (set_ip_fla(sock) < 0) {
        return EXIT_FAILURE;
    }
    
    
    // create packet


    // send low entropy train



    // send high entropy train



    // compare results to detect compression


}
