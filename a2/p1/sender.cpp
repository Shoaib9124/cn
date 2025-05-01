#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

using namespace std;

int main() {
    int sockfd, port;
    char ip[100], data[BUFFER_SIZE], frame[BUFFER_SIZE];
    struct sockaddr_in receiver_addr;

    cout << "Enter receiver IP (e.g., 127.0.0.1): ";
    cin >> ip;
    cout << "Enter receiver port: ";
    cin >> port;
    cin.ignore(); // Clear newline

    cout << "Enter data to send: ";
    cin.getline(data, BUFFER_SIZE);

    snprintf(frame, BUFFER_SIZE, "<FRAME>%s</FRAME>", data);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &receiver_addr.sin_addr);

    if (sendto(sockfd, frame, strlen(frame), 0,
               (struct sockaddr*)&receiver_addr, sizeof(receiver_addr)) < 0) {
        perror("sendto");
    } else {
        cout << "[Sender] Frame sent: " << frame << endl;
    }

    close(sockfd);
    return 0;
}
