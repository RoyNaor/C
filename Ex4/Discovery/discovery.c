#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include "discovery.h"
#include <sys/time.h>
#include <getopt.h>

/**
 * @brief Function to check if a host is active by sending an ICMP echo request.
 * @param ip The IP address of the target host.
 * @return 1 if the host is active, 0 otherwise.
 */
int is_host_active(const char *ip) {
    // Create a raw socket for ICMP
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        perror("Socket creation failed");
        return 0;
    }

    // Prepare the target address structure
    struct sockaddr_in target;
    memset(&target, 0, sizeof(target)); // Clear memory for safety
    target.sin_family = AF_INET;

    // Convert the IP address to binary format
    if (inet_pton(AF_INET, ip, &target.sin_addr) <= 0) {
        close(sock);
        return 0;
    }

    // Create and initialize the ICMP header
    struct icmphdr icmp_hdr;
    memset(&icmp_hdr, 0, sizeof(icmp_hdr)); // Zero out the ICMP header
    icmp_hdr.type = ICMP_ECHO; // Set ICMP type to ECHO request
    icmp_hdr.code = 0; // Code is always 0 for ICMP ECHO
    icmp_hdr.un.echo.id = htons(getpid()); // Use process ID as unique identifier
    icmp_hdr.un.echo.sequence = 0; // Sequence number starts at 0
    icmp_hdr.checksum = calculate_checksum(&icmp_hdr, sizeof(icmp_hdr)); // Calculate checksum

    // Send the ICMP echo request
    if (sendto(sock, &icmp_hdr, sizeof(icmp_hdr), 0, (struct sockaddr *)&target, sizeof(target)) <= 0) {
        perror("Sendto failed");
        close(sock);
        return 0;
    }

    // Wait for an ICMP echo reply with a timeout
    struct timeval timeout = {0, 200000}; // 200ms timeout
    fd_set read_fds;
    FD_ZERO(&read_fds); // Initialize file descriptor set
    FD_SET(sock, &read_fds); // Add socket to the set

    // Wait for socket readiness
    int result = select(sock + 1, &read_fds, NULL, NULL, &timeout);

    // Process the reply if available
    if (result > 0) {
        char buffer[1024];
        struct sockaddr_in reply_addr;
        socklen_t addr_len = sizeof(reply_addr);

        if (recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&reply_addr, &addr_len) > 0) {
            close(sock); // Close the socket
            return 1; // Host is active
        }
    }

    close(sock); // Close the socket
    return 0; // Host is inactive
}

/**
 * @brief Calculate checksum for ICMP packets.
 * @param data Pointer to the data.
 * @param bytes Number of bytes in the data.
 * @return The calculated checksum.
 */
unsigned short int calculate_checksum(void *data, unsigned int bytes) {
    unsigned short int *data_pointer = (unsigned short int *)data;
    unsigned int total_sum = 0;

    // Add up 16-bit values in the data
    while (bytes > 1) {
        total_sum += *data_pointer++;
        bytes -= 2;
    }

    // Add any remaining byte (if odd length)
    if (bytes > 0) {
        total_sum += *((unsigned char *)data_pointer);
    }

    // Fold 32-bit sum to 16 bits
    while (total_sum >> 16) {
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
    }

    return (~((unsigned short int)total_sum)); // Return the one's complement
}

/**
 * @brief Main function to perform network scanning.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char *argv[]) {
    char *address = NULL; // Store the base network address
    int subnet = 0; // Store the subnet mask
    int opt;

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "a:c:")) != -1) {
        switch (opt) {
            case 'a':
                address = optarg; // Store the network address
                break;
            case 'c':
                subnet = atoi(optarg); // Convert subnet mask to integer
                break;
            default:
                fprintf(stderr, "Usage: %s -a <address> -c <subnet>\n", argv[0]);
                return 1;
        }
    }

    // Validate input arguments
    if (!address || subnet <= 0 || subnet > 32) {
        fprintf(stderr, "Error: Invalid arguments. Please provide a valid address and subnet mask.\n");
        return 1;
    }

    // Validate IP address format
    struct in_addr base_addr;
    if (inet_pton(AF_INET, address, &base_addr) <= 0) {
        fprintf(stderr, "Error: Invalid IPv4 address format: %s\n", address);
        return 1;
    }

    printf("Scanning network %s/%d:\n", address, subnet); // Print scan details

    // Calculate the total number of hosts in the subnet
    unsigned int host_count = 1 << (32 - subnet); // Total hosts in subnet
    unsigned int base_ip = ntohl(base_addr.s_addr); // Convert base IP to host byte order
    unsigned int max_ip = base_ip + host_count; // Calculate max IP in range

    // Iterate through each IP address in the subnet
    for (unsigned int ip = base_ip + 1; ip < max_ip - 1; ip++) { // Exclude network and broadcast
        struct in_addr current_addr;
        current_addr.s_addr = htonl(ip); // Convert back to network byte order

        // Convert IP address to string format
        char ip_str[INET_ADDRSTRLEN];
        if (!inet_ntop(AF_INET, &current_addr, ip_str, INET_ADDRSTRLEN)) {
            continue;
        }

        // Check if the host is active
        if (is_host_active(ip_str)) {
            printf("%s\n", ip_str); // Print active host
        }
    }

    printf("Scan Complete!\n"); // Indicate the end of the scan
    return 0;
}
