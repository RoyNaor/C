#ifndef TRACEROUTE_H
#define TRACEROUTE_H

// Constants
#define TIMEOUT 1000          // Timeout for responses (in ms)
#define MAX_HOPS 30           // Maximum TTL value
#define PACKETS_PER_HOP 3     // Packets sent per hop
#define BUFFER_SIZE 1024      // Buffer size for packets

// Function prototypes
unsigned short int calculate_checksum(void *data, unsigned int bytes);

#endif // TRACEROUTE_H
