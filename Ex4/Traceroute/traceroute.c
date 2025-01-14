#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <poll.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include "traceroute.h"

// Function prototypes
unsigned short int calculate_checksum(void *data, unsigned int bytes);
double calculate_rtt(struct timeval *start, struct timeval *end);

int main(int argc, char *argv[]) {
    int opt;
    char *address = NULL;
    int ttl = 1;

    // Parse command-line arguments to get the target address
    while ((opt = getopt(argc, argv, "a:")) != -1) {
        switch (opt) {
            case 'a':
                address = optarg; // Assign the provided address
                break;
            default:
                fprintf(stderr, "Usage: %s -a <address>\n", argv[0]);
                return 1;
        }
    }

    // Ensure the address is provided
    if (!address) {
        fprintf(stderr, "Error: -a <address> is required.\n");
        return 1;
    }

    // Set up the destination address structure
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, address, &dest_addr.sin_addr) <= 0) {
        fprintf(stderr, "Error: \"%s\" is not a valid IPv4 address.\n", address);
        return 1;
    }

    // Create a raw socket for ICMP
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        perror("socket");
        if (errno == EACCES || errno == EPERM) {
            fprintf(stderr, "You need to run this program with sudo.\n");
        }
        return 1;
    }

    // Print the traceroute header
    fprintf(stdout, "Traceroute to %s, %d hops max:\n", address, MAX_HOPS);

    // Loop over each TTL (Time to Live)
    for (ttl = 1; ttl <= MAX_HOPS; ++ttl) {
        // Set the TTL value for the current hop
        if (setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) != 0) {
            perror("setsockopt");
            close(sock);
            return 1;
        }

        fprintf(stdout, "%2d  ", ttl); // Print the current hop number
        int success = 0; // Track if any packet was successful

        // Send multiple packets for each hop
        for (int i = 0; i < PACKETS_PER_HOP; ++i) {
            char buffer[BUFFER_SIZE] = {0}; // Buffer to hold the ICMP packet
            struct icmphdr icmp_hdr; // ICMP header
            icmp_hdr.type = ICMP_ECHO;
            icmp_hdr.code = 0;
            icmp_hdr.un.echo.id = htons(getpid()); // Use process ID as identifier
            icmp_hdr.un.echo.sequence = htons(ttl * PACKETS_PER_HOP + i); // Sequence number
            icmp_hdr.checksum = 0;
            memcpy(buffer, &icmp_hdr, sizeof(icmp_hdr));
            icmp_hdr.checksum = calculate_checksum(buffer, sizeof(icmp_hdr));
            ((struct icmphdr *)buffer)->checksum = icmp_hdr.checksum;

            struct timeval start, end; // Timestamps for RTT calculation
            gettimeofday(&start, NULL); // Record start time

            // Send the ICMP packet
            if (sendto(sock, buffer, sizeof(icmp_hdr), 0,
                       (struct sockaddr *)&dest_addr, sizeof(dest_addr)) <= 0) {
                perror("sendto");
                fprintf(stdout, "* "); // Indicate failure for this packet
                continue;
            }

            // Wait for a response with a timeout
            struct pollfd fds[1];
            fds[0].fd = sock;
            fds[0].events = POLLIN;

            int ret = poll(fds, 1, TIMEOUT);
            if (ret == 0) { // Timeout occurred
                fprintf(stdout, "* ");
                continue;
            } else if (ret < 0) { // Error during poll
                perror("poll");
                fprintf(stdout, "* ");
                continue;
            }

            // Receive the ICMP reply
            char reply[BUFFER_SIZE];
            struct sockaddr_in reply_addr;
            socklen_t addr_len = sizeof(reply_addr);
            if (recvfrom(sock, reply, sizeof(reply), 0,
                         (struct sockaddr *)&reply_addr, &addr_len) <= 0) {
                perror("recvfrom");
                fprintf(stdout, "* ");
                continue;
            }

            gettimeofday(&end, NULL); // Record end time
            double rtt = calculate_rtt(&start, &end); // Calculate RTT

            if (i == 0) { // Print the IP address only once per hop
                fprintf(stdout, "%s ", inet_ntoa(reply_addr.sin_addr));
            }
            fprintf(stdout, "%.3fms ", rtt); // Print the RTT for this packet
            success = 1; // Mark the hop as successful

            // If the destination is reached, exit the loop
            if (memcmp(&dest_addr.sin_addr, &reply_addr.sin_addr, sizeof(dest_addr.sin_addr)) == 0) {
                fprintf(stdout, "\n");
                close(sock);
                return 0;
            }
        }

        // Print a newline after each hop
        if (!success) {
            fprintf(stdout, "\n"); // If all packets failed, just a newline
        } else {
            fprintf(stdout, "\n"); // If some packets succeeded, add a newline
        }
    }

    close(sock); // Close the socket
    return 0;
}

// Calculate checksum for IP or ICMP headers
unsigned short int calculate_checksum(void *data, unsigned int bytes) {
    unsigned short int *data_pointer = (unsigned short int *)data;
    unsigned int total_sum = 0;

    while (bytes > 1) { // Process two bytes at a time
        total_sum += *data_pointer++;
        bytes -= 2;
    }

    if (bytes > 0) { // If there is a leftover byte
        total_sum += *((unsigned char *)data_pointer);
    }

    while (total_sum >> 16) { // Fold 32-bit sum to 16 bits
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
    }

    return (~((unsigned short int)total_sum)); // Return the one's complement
}

// Calculate RTT between two time points
double calculate_rtt(struct timeval *start, struct timeval *end) {
    return (end->tv_sec - start->tv_sec) * 1000.0 + (end->tv_usec - start->tv_usec) / 1000.0; // Convert to milliseconds
}
