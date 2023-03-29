/**
 * @file
 *
 * Defines compression detection helper functions.
 */

#ifndef _UTIL_H_
#define _UTIL_H_

char* read_file(char *filename, int size);
void set_packet_id(char *payload, int id);
char* create_low_entropy_payload(int id, int payload_size);
char* create_high_entropy_payload(int id, int payload_size);
double time_diff_milli(struct timeval tv1, struct timeval tv2);
double time_diff_sec(struct timeval tv1, struct timeval tv2);
void print_packet(char* packet, int size);

#endif
