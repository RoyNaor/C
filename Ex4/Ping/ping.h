#ifndef _PING_H  // Header guard to prevent multiple inclusions of this header file
#define _PING_H  // Start of the header guard definition

#define TIMEOUT 2000  // Timeout for network operations in milliseconds
#define BUFFER_SIZE 1024  // Size of the buffer used for sending/receiving data in bytes
#define SLEEP_TIME 1  // Sleep time between consecutive ping requests in seconds
#define MAX_REQUESTS 0  // Maximum number of ping requests to send (0 means unlimited)
#define MAX_RETRY 3  // Maximum number of retries in case of failure

// Function declaration for calculating the checksum of ICMP packets
unsigned short int calculate_checksum(void *data, unsigned int bytes);

#endif // _PING_H