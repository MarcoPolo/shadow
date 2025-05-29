#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>

#define NUM_PACKETS 20
#define PACKET_SIZE 64
#define SERVER_PORT 8080

static long long get_timestamp_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

void run_server() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[PACKET_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    printf("Server listening on port %d\n", SERVER_PORT);

    for (int i = 0; i < NUM_PACKETS; i++) {
        ssize_t n = recvfrom(sockfd, buffer, PACKET_SIZE, 0,
                            (struct sockaddr*)&client_addr, &client_len);
        if (n > 0) {
            long long timestamp = get_timestamp_us();
            printf("Received packet %d at %lld us\n", i, timestamp);

            // Echo back to client
            sendto(sockfd, buffer, n, 0,
                  (struct sockaddr*)&client_addr, client_len);
        }
    }

    close(sockfd);
}

void run_client(const char* hostname) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[PACKET_SIZE];
    long long send_times[NUM_PACKETS];
    long long recv_times[NUM_PACKETS];
    long long rtts[NUM_PACKETS];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Check if hostname is an IP address or needs resolution
    server_addr.sin_addr.s_addr = inet_addr(hostname);
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        // Not a valid IP address, try hostname resolution
        struct hostent *he = gethostbyname(hostname);
        if (he == NULL) {
            fprintf(stderr, "Failed to resolve hostname: %s\n", hostname);
            exit(1);
        }
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
        printf("Resolved %s to %s\n", hostname, inet_ntoa(server_addr.sin_addr));
    } else {
        printf("Using IP address: %s\n", hostname);
    }

    printf("Client sending %d packets\n", NUM_PACKETS);

    for (int i = 0; i < NUM_PACKETS; i++) {
        snprintf(buffer, PACKET_SIZE, "packet_%d", i);

        send_times[i] = get_timestamp_us();
        sendto(sockfd, buffer, strlen(buffer), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr));

        ssize_t n = recvfrom(sockfd, buffer, PACKET_SIZE, 0, NULL, NULL);
        recv_times[i] = get_timestamp_us();

        if (n > 0) {
            rtts[i] = recv_times[i] - send_times[i];
            printf("Packet %d RTT: %lld us\n", i, rtts[i]);
        }

        usleep(10000); // 10ms between packets
    }

    // Calculate jitter statistics
    long long total_rtt = 0;
    for (int i = 0; i < NUM_PACKETS; i++) {
        total_rtt += rtts[i];
    }
    long long avg_rtt = total_rtt / NUM_PACKETS;

    long long jitter_sum = 0;
    for (int i = 1; i < NUM_PACKETS; i++) {
        long long diff = rtts[i] - rtts[i-1];
        if (diff < 0) diff = -diff;
        jitter_sum += diff;
    }
    long long avg_jitter = jitter_sum / (NUM_PACKETS - 1);

    printf("Average RTT: %lld us\n", avg_rtt);
    printf("Average Jitter: %lld us\n", avg_jitter);

    close(sockfd);

    // Fail the test if average jitter is 0
    if (avg_jitter == 0) {
        printf("Test failed: Average jitter is 0\n");
        exit(2);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2 || (strcmp(argv[1], "client") == 0 && argc != 3)) {
        printf("Usage: %s server\n", argv[0]);
        printf("   or: %s client <hostname>\n", argv[0]);
        exit(1);
    }
    printf("Starting %s...\n", argv[1]);

    if (strcmp(argv[1], "server") == 0) {
        run_server();
    } else if (strcmp(argv[1], "client") == 0) {
        sleep(1); // Wait for server to start
        run_client(argv[2]);
    } else {
        printf("Invalid argument. Use 'server' or 'client'\n");
        exit(1);
    }

    return 0;
}
