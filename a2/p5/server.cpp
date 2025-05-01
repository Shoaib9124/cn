#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define BUFFER_SIZE 1024

using namespace std;

// Global variable to handle graceful exit
volatile bool running = true;

// Signal handler for Ctrl+C
void signalHandler(int signum) {
    cout << "\n[Receiver] Shutting down..." << endl;
    running = false;
}

// Send ACK with buffer size framing bit
void sendAck(int sockfd, sockaddr_in &senderAddr, socklen_t addr_len, int ackNo, int bufferSize) {
    char ackMsg[BUFFER_SIZE];
    snprintf(ackMsg, sizeof(ackMsg), "ACK %d BUFFER %d", ackNo, bufferSize);
    sendto(sockfd, ackMsg, strlen(ackMsg), 0, (struct sockaddr*)&senderAddr, addr_len);
    cout << "[Receiver] Sent: " << ackMsg << endl;
}

int main() {
    string portStr;
    int port;
    int bufferSize;
    
    cout << "Enter port to listen on (default: 12345): ";
    getline(cin, portStr);
    if (portStr.empty()) {
        port = 12345;
    } else {
        port = stoi(portStr);
    }

    cout << "Enter receiver buffer size (1-7, default: 4): ";
    string bufferSizeStr;
    getline(cin, bufferSizeStr);
    if (bufferSizeStr.empty()) {
        bufferSize = 4;
    } else {
        bufferSize = stoi(bufferSizeStr);
        // Ensure buffer size is within reasonable bounds
        if (bufferSize < 1) bufferSize = 1;
        if (bufferSize > 7) bufferSize = 7;
    }

    int sockfd;
    sockaddr_in recvAddr{}, senderAddr{};
    socklen_t addr_len = sizeof(senderAddr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    recvAddr.sin_family = AF_INET;
    recvAddr.sin_addr.s_addr = INADDR_ANY;
    recvAddr.sin_port = htons(port);

    if (bind(sockfd, (sockaddr*)&recvAddr, sizeof(recvAddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return 1;
    }

    // Set up signal handler for graceful termination
    signal(SIGINT, signalHandler);

    cout << "[Receiver] Listening on port " << port << " with buffer size " << bufferSize << endl;
    cout << "[Receiver] Waiting for frames... (Press Ctrl+C to exit)" << endl;

    int expectedFrame = 0;
    
    // Set a timeout for recvfrom to allow for periodic checking of running flag
    struct timeval tv;
    tv.tv_sec = 1;  // 1 second timeout
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (running) {
        char buffer[BUFFER_SIZE] = {0};
        ssize_t len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (sockaddr*)&senderAddr, &addr_len);

        if (len > 0) {
            buffer[len] = '\0';
            cout << "[Receiver] Received: " << buffer << endl;

            // Check if it's an exit signal
            if (strcmp(buffer, "EXIT") == 0) {
                cout << "[Receiver] Received exit signal. Shutting down." << endl;
                break;
            }

            int frameNo;
            if (sscanf(buffer, "FRAME %d", &frameNo) == 1) {
                if (frameNo == expectedFrame) {
                    cout << "[Receiver] Frame accepted: " << frameNo << endl;
                    sendAck(sockfd, senderAddr, addr_len, frameNo, bufferSize);
                    expectedFrame++;
                } else if (frameNo < expectedFrame) {
                    cout << "[Receiver] Duplicate frame " << frameNo << ". Resending ACK." << endl;
                    sendAck(sockfd, senderAddr, addr_len, frameNo, bufferSize);
                } else {
                    cout << "[Receiver] Out of order frame. Expected " << expectedFrame << ", got " << frameNo << endl;
                    // Send ACK for the last correctly received frame
                    if (expectedFrame > 0) {
                        sendAck(sockfd, senderAddr, addr_len, expectedFrame - 1, bufferSize);
                    }
                }
            }
        } else if (len < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            // Real error (not timeout)
            perror("recvfrom error");
        }
        // If len == 0 or EAGAIN/EWOULDBLOCK, it's just a timeout, continue loop
    }

    cout << "[Receiver] Shutting down gracefully." << endl;
    close(sockfd);
    return 0;
}
