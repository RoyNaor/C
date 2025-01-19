#ifndef DISCOVERY_H
#define DISCOVERY_H

// Function declarations
unsigned short int calculate_checksum(void *data, unsigned int bytes);
int is_host_active(const char *ip);

#endif // DISCOVERY_H
