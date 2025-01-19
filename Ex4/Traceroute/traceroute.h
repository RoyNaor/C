#ifndef TRACEROUTE_H
#define TRACEROUTE_H

#include <netinet/ip.h>
#include <netinet/in.h>
#include <sys/time.h>

// Constants
#define TIMEOUT 1000          // Timeout for responses (in ms)
#define MAX_HOPS 30           // Maximum TTL value
#define PACKETS_PER_HOP 3     // Packets sent per hop
#define BUFFER_SIZE 1024      // Buffer size for packets

// Function prototypes
unsigned short int calculate_checksum(void *data, unsigned int bytes);
double calculate_rtt(struct timeval *start, struct timeval *end);
void build_ip_header(struct iphdr *ip_hdr, struct sockaddr_in *dest_addr, int ttl, int payload_len);

#endif // TRACEROUTE_H
