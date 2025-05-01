#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <random>

#define BUFFER_SIZE 1024

using namespace std;

bool simulateLoss(double probability) {
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) > probability;
}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    string portStr;
    int serverPort;

    cout << "Enter port to listen on (default: 12345): ";
    getline(cin, portStr);
    serverPort = portStr.empty() ? 12345 : stoi(portStr);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return 1;
    }

    cout << "Server listening on port " << serverPort << endl;

    int mode;
    cout << "Receiver started.\nSelect mode:\n1. Noiseless\n2. Noisy\nEnter choice: ";
    cin >> mode;
    cin.ignore();  // Clear newline

    while (true) {
        socklen_t len = sizeof(clientAddr);
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&clientAddr, &len);
        if (n < 0) {
            perror("recvfrom failed");
            break;
        }

        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';  // Remove newline

        if (strcmp(buffer, "exit") == 0) {
            cout << "[Receiver] Exit command received. Shutting down.\n";
            break;
        }

        if (mode == 1 || simulateLoss(0.8)) {
            cout << "[Receiver] Received: " << buffer << endl;
            char ack[BUFFER_SIZE];
            snprintf(ack, sizeof(ack), "ACK for %s", buffer);
            sendto(sockfd, ack, strlen(ack), 0, (struct sockaddr *)&clientAddr, len);
            cout << "[Receiver] Sent: " << ack << endl;
        } else {
            cout << "[Receiver] Frame lost during reception.\n";
        }

        usleep(200000);
    }

    close(sockfd);
    return 0;
}
