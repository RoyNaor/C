#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>      // For inet_pton, inet_ntoa
#include <netinet/in.h>     // For sockaddr_in
#include <netinet/ip.h>     // For IP header
#include <netinet/ip_icmp.h> // For ICMP header
#include <poll.h>           // For poll
#include <errno.h>          // For error handling
#include <sys/socket.h>     // For socket operations
#include <sys/time.h>       // For gettimeofday
#include <unistd.h>         // For getpid, close, sleep
#include <signal.h>         // For signal handling
#include <getopt.h>         // For getopt
#include <math.h>           // For sqrt
#include "ping.h"           // Custom functions/constants

// Global variables for statistics
volatile int packets_sent = 0, packets_received = 0; // Packet counters
volatile float rtt_min = 0, rtt_max = 0, rtt_sum = 0, rtt_squared_sum = 0; // Round-trip time metrics

// Signal handler to print statistics when program is interrupted (Ctrl+C)
void handle_sigint() {
    float rtt_avg = (packets_received > 0) ? (rtt_sum / packets_received) : 0; // Calculate average RTT
    float rtt_mdev = (packets_received > 0) ? sqrt((rtt_squared_sum / packets_received) - (rtt_avg * rtt_avg)) : 0; // Calculate mdev RTT

    fprintf(stdout, "\n--- Statistics ---\n");
    fprintf(stdout, "%d packets transmitted, %d received\n", packets_sent, packets_received);
    fprintf(stdout, "rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n", 
            rtt_min, rtt_avg, rtt_max, rtt_mdev);
    exit(0); // Exit program
}

int main(int argc, char *argv[]) {
    // Register signal handler for Ctrl+C
    signal(SIGINT, handle_sigint);

    // Variable declarations for command-line arguments
    int opt;               // Option character for getopt
    char *address = NULL;  // Target address
    int type = 0;          // Address type (4 for IPv4, 6 for IPv6)
    int count = MAX_REQUESTS; // Number of packets to send (default: unlimited)
    int flood = 0;         // Flood mode flag

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "a:t:c:f")) != -1) {
        switch (opt) {
            case 'a':
                address = optarg; // Store target address
                break;
            case 't':
                if (strcmp(optarg, "4") == 0) {
                    type = 4; // IPv4
                } else if (strcmp(optarg, "6") == 0) {
                    type = 6; // IPv6
                } else {
                    fprintf(stderr, "Error: Invalid type. Use '4' for IPv4 or '6' for IPv6.\n");
                    return 1;
                }
                break;
            case 'c':
                count = atoi(optarg); // Number of packets
                if (count <= 0) {
                    fprintf(stderr, "Error: Count must be a positive integer.\n");
                    return 1;
                }
                break;
            case 'f':
                flood = 1; // Enable flood mode
                break;
            default:
                fprintf(stderr, "Usage: %s -a <address> -t <type> [-c <count>] [-f]\n", argv[0]);
                return 1;
        }
    }

    // Validate required arguments
    if (!address || type == 0) {
        fprintf(stderr, "Error: Both -a (address) and -t (type) flags are required.\n");
        return 1;
    }

    // Set up the destination address
    struct sockaddr_storage destination_address;
    memset(&destination_address, 0, sizeof(destination_address));

    if (type == 4) { // IPv4 setup
        struct sockaddr_in *addr4 = (struct sockaddr_in *)&destination_address;
        addr4->sin_family = AF_INET;
        if (inet_pton(AF_INET, address, &addr4->sin_addr) <= 0) {
            fprintf(stderr, "Error: \"%s\" is not a valid IPv4 address\n", address);
            return 1;
        }
    } else { // IPv6 setup
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&destination_address;
        addr6->sin6_family = AF_INET6;
        if (inet_pton(AF_INET6, address, &addr6->sin6_addr) <= 0) {
            fprintf(stderr, "Error: \"%s\" is not a valid IPv6 address\n", address);
            return 1;
        }
    }

    // Create the raw socket
    int sock;
    if (type == 4) {
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); // IPv4 socket
    } else {
        sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6); // IPv6 socket
    }

    if (sock < 0) {
        perror("socket");
        if (errno == EACCES || errno == EPERM) {
            fprintf(stderr, "You need to run the program with sudo.\n");
        }
        return 1;
    }

    // Initialize ICMP header
    struct icmphdr icmp_header;
    icmp_header.type = ICMP_ECHO; // ICMP Echo Request
    icmp_header.code = 0;         // No additional code
    icmp_header.un.echo.id = htons(getpid()); // Unique identifier
    int seq = 0; // Sequence number for ICMP packets

    struct pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLIN; // Monitor for incoming packets

    fprintf(stdout, "Pinging %s with %zu bytes of data:\n", address, sizeof(icmp_header));

    // Main loop for sending packets
    while (1) {
        if (count > 0 && seq >= count) { // Stop if count is reached
            break;
        }

        packets_sent++; // Increment packet sent counter

        // Prepare ICMP packet
        char buffer[BUFFER_SIZE] = {0};
        icmp_header.un.echo.sequence = htons(seq++); // Set sequence number
        icmp_header.checksum = 0; // Reset checksum
        memcpy(buffer, &icmp_header, sizeof(icmp_header)); // Copy ICMP header to buffer
        icmp_header.checksum = calculate_checksum(buffer, sizeof(icmp_header)); // Calculate checksum
        ((struct icmphdr *)buffer)->checksum = icmp_header.checksum; // Set checksum

        struct timeval start, end; // Track packet round-trip time
        gettimeofday(&start, NULL);

        // Determine address length based on type
        size_t addr_len;
        if (type == 4) {
            addr_len = sizeof(struct sockaddr_in);
        } else {
            addr_len = sizeof(struct sockaddr_in6);
        }

        // Send the ICMP packet
        if (sendto(sock, buffer, sizeof(icmp_header), 0, 
                   (struct sockaddr *)&destination_address, addr_len) <= 0) {
            perror("sendto");
            continue;
        }

        // Wait for a response
        int ret = poll(fds, 1, TIMEOUT);

        if (ret == 0) { // Timeout
            fprintf(stderr, "Request timeout for icmp_seq %d\n", seq);
            continue;
        } else if (ret < 0) { // Error in poll
            perror("poll");
            continue;
        }

        // Process incoming packet
        if (fds[0].revents & POLLIN) {
            char reply_buffer[BUFFER_SIZE]; // Buffer for reply packet
            struct sockaddr_storage source_address; // Address of the reply source
            socklen_t src_len = sizeof(source_address);

            if (recvfrom(sock, reply_buffer, sizeof(reply_buffer), 0, 
                         (struct sockaddr *)&source_address, &src_len) <= 0) {
                perror("recvfrom");
                continue;
            }

            gettimeofday(&end, NULL); // Record end time
            float elapsed = ((end.tv_sec - start.tv_sec) * 1000.0) + 
                            ((end.tv_usec - start.tv_usec) / 1000.0); // Calculate RTT
            rtt_sum += elapsed; // Update RTT sum
            rtt_squared_sum += elapsed * elapsed; // Update squared RTT sum

            if (packets_received == 0 || elapsed < rtt_min) { // Update min RTT
                rtt_min = elapsed;
            }

            if (elapsed > rtt_max) { // Update max RTT
                rtt_max = elapsed;
            }

            packets_received++; // Increment received packet count

            // Print reply details
            fprintf(stdout, "%ld bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n", 
                    (long)(sizeof(icmp_header)), 
                    address, ntohs(icmp_header.un.echo.sequence), 
                    64, elapsed);
        }

        if (!flood) { // Add delay if flood mode is not enabled
            sleep(SLEEP_TIME);
        }
    }

    handle_sigint(); // Call handler for final statistics

    return 0;
}

// Calculate checksum for ICMP header
unsigned short int calculate_checksum(void *data, unsigned int bytes) {
    unsigned short int *data_pointer = (unsigned short int *)data;
    unsigned int total_sum = 0;

    while (bytes > 1) { // Sum 16-bit words
        total_sum += *data_pointer++;
        bytes -= 2;
    }

    if (bytes > 0) { // Add any remaining byte
        total_sum += *((unsigned char *)data_pointer);
    }

    while (total_sum >> 16) { // Fold carry bits into lower 16 bits
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
    }

    return (~((unsigned short int)total_sum)); // Return one's complement
}
