// sender.cpp
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <random>
#include <sys/time.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define DEFAULT_FRAMES 10

using namespace std;

bool simulateLoss(double probability) {
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) > probability;
}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    string serverIP;
    int serverPort;

    cout << "Enter server IP address (default: 127.0.0.1): ";
    getline(cin, serverIP);
    if (serverIP.empty()) serverIP = "127.0.0.1";

    cout << "Enter server port (default: 12345): ";
    string portStr;
    getline(cin, portStr);
    serverPort = portStr.empty() ? 12345 : stoi(portStr);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());

    // Set timeout on socket for receiving
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000; // 500 ms
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    int mode;
    cout << "Select mode:\n1. Noiseless\n2. Noisy\nEnter choice: ";
    cin >> mode;
    cin.ignore();

    int totalFrames = DEFAULT_FRAMES;
    cout << "Sending " << totalFrames << " frames...\n";

    for (int i = 1; i <= totalFrames;) {
        sprintf(buffer, "Frame %d", i);

        if (mode == 1 || simulateLoss(0.8)) {
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            cout << "[Sender] Sent: " << buffer << endl;
        } else {
            cout << "[Sender] Frame " << i << " lost during transmission.\n";
        }

        // Wait for ACK
        socklen_t addrLen = sizeof(serverAddr);
        ssize_t len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&serverAddr, &addrLen);
        if (len > 0) {
            buffer[len] = '\0';
            cout << "[Sender] Received: " << buffer << "\n";
            i++; // Move to next frame only if ACK received
        } else {
            cout << "[Sender] ACK lost or timeout. Resending frame " << i << "\n";
        }

        usleep(200000); // 200 ms
    }

    const char* exitMsg = "exit";
    sendto(sockfd, exitMsg, strlen(exitMsg), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    cout << "[Sender] Sent termination signal: " << exitMsg << endl;

    close(sockfd);
    return 0;
}
