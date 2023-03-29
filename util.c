/**
 * @file
 *
 * Contains compression detection helper functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

/**
 * Reads in file
 *
 * filename: name (+ path) of file to read in
 * size: size of file
 *
 * returns: char pointer to contents if successful, NULL otherwise
 */
char* read_file(char *filename, int size)
{
    // malloc buffer size
    char *buf = malloc(size);
    if (buf == NULL) {
        perror("Error when mallocing buf");
        return NULL;
    }

    // open file
    FILE *fp;
    if ((fp = fopen(filename, "r")) < 0) {
        perror("Error when opening file");
        free(buf);
        return NULL;
    }

    // read file
    if (fread(buf, size, 1, fp) < 0) {
        perror("Error when reading file");
        free(buf);
        return NULL;
    }
    fclose(fp);

    return buf;
}

/**
 * Sets the id of a packet (first two bytes)
 *
 * payload: pointer to char array of packet
 * int: id to set
 */
void set_packet_id(char *payload, int id)
{
    if (id <= 255) {
        memset(payload, 0, 1);
        memset(payload + 1, id, 1);
    } else {
        memset(payload + 1, id & 255, 1);
        id >>= 8;
        memset(payload, id & 255, 1);
    }
}

/**
 * Creates low entropy payload
 *
 * id: id of packet
 * payload_size: size of payload
 *
 * returns: char pointer to payload if successful, NULL otherwise
 */
char* create_low_entropy_payload(int id, int payload_size)
{
    char *payload = malloc(payload_size);
    if (payload == NULL) {
        perror("Error mallocing payload");
        return NULL;
    }
    memset(payload, 0, payload_size);

    // set packet id
    set_packet_id(payload, id);

    return payload;
}

/**
 * Creates high entropy payload
 *
 * id: id of packet
 * payload_size: size of payload
 *
 * returns: char pointer to payload if successful, NULL otherwise
 */
char* create_high_entropy_payload(int id, int payload_size)
{
    char *payload = read_file("myrandom", payload_size);
    if (payload == NULL) {
        return NULL;
    }

    // set packet id
    set_packet_id(payload, id);

    return payload;
}

/**
 * Finds the difference in milliseconds between two timeval structs (tv1 - tv2)
 *
 * tv1: struct timeval
 * tv2: sturct timeval
 *
 * returns: double
 */
double time_diff_milli(struct timeval tv1, struct timeval tv2)
{
    double tv1_mili = (tv1.tv_sec * 1000) + (tv1.tv_usec / 1000);
    double tv2_mili = (tv2.tv_sec * 1000) + (tv2.tv_usec / 1000);
    return tv1_mili - tv2_mili;
}

/**
 * Finds the difference in seconds between two timeval structs (tv1 - tv2)
 *
 * tv1: struct timeval
 * tv2: sturct timeval
 *
 * returns: double
 */
double time_diff_sec(struct timeval tv1, struct timeval tv2)
{
    double tv1_sec = tv1.tv_sec + (tv1.tv_usec / 1000000);
    double tv2_sec = tv2.tv_sec + (tv2.tv_usec / 1000000);
    return tv1_sec - tv2_sec;
}

/**
 * Prints the binary representation of a packet, 4 bytes a row
 *
 * packet: char pointer to packet
 * size: size of packet
 */
void print_packet(char* packet, int size)
{
    for (int i = 0; i < size; ++i) {
        // print each byte as bits
        for (int j = 7; 0 <= j; j--) {
            printf("%c", (packet[i] & (1 << j)) ? '1' : '0');
        }   
        // print spaces
        if ((i+1) % 4 == 0) {
            printf("\n");
        } else {
            printf(" ");
        }
    }
}
