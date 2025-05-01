#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

using namespace std;

int main() {
    int sockfd, port;
    struct sockaddr_in recv_addr, sender_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(sender_addr);

    cout << "Enter port to listen on: ";
    cin >> port;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = INADDR_ANY;
    recv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    cout << "[Receiver] Waiting for a frame..." << endl;

    ssize_t len = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                           (struct sockaddr*)&sender_addr, &addr_len);
    if (len < 0) {
        perror("recvfrom");
        close(sockfd);
        return 1;
    }

    buffer[len] = '\0';

    // Simple parsing (assumes valid format)
    const char* start = strstr(buffer, "<FRAME>");
    const char* end = strstr(buffer, "</FRAME>");

    if (start && end && end > start) {
        char data[BUFFER_SIZE] = {0};
        strncpy(data, start + 7, end - (start + 7));  // Skip "<FRAME>"
        cout << "[Receiver] Delivered data: " << data << endl;
    } else {
        cout << "[Receiver] Invalid frame received.\n";
    }

    close(sockfd);
    return 0;
}
