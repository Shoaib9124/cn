#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024
#define MAX_FRAMES 10
#define TIMEOUT_MS 5000  // 5 second timeout

using namespace std;

// Send frame
void sendFrame(int sockfd, sockaddr_in &receiverAddr, int frameNo) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "FRAME %d", frameNo);
    sendto(sockfd, buffer, strlen(buffer), 0, (sockaddr*)&receiverAddr, sizeof(receiverAddr));
    cout << "[Sender] Sent: " << buffer << endl;
}

// Send exit signal
void sendExit(int sockfd, sockaddr_in &receiverAddr) {
    const char* exitMsg = "EXIT";
    sendto(sockfd, exitMsg, strlen(exitMsg), 0, (sockaddr*)&receiverAddr, sizeof(receiverAddr));
    cout << "[Sender] Sent exit signal to receiver" << endl;
}

// Receive ACK with buffer info - non-blocking version
bool receiveAck(int sockfd, int &ackNo, int &receiverBufferSize) {
    char buffer[BUFFER_SIZE];
    socklen_t addr_len;
    ssize_t len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);

    if (len > 0) {
        buffer[len] = '\0';
        if (sscanf(buffer, "ACK %d BUFFER %d", &ackNo, &receiverBufferSize) == 2) {
            cout << "[Sender] Received: " << buffer << endl;
            return true;
        }
    }
    return false;
}

int main() {
    string ip;
    string portStr;
    int port;

    cout << "Enter receiver IP (default: 127.0.0.1): ";
    getline(cin, ip);
    if (ip.empty()) {
        ip = "127.0.0.1";
    }

    cout << "Enter receiver port (default: 12345): ";
    getline(cin, portStr);
    if (portStr.empty()) {
        port = 12345;
    } else {
        port = stoi(portStr);
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Set socket to non-blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in receiverAddr{};
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &receiverAddr.sin_addr);

    cout << "[Sender] Connecting to " << ip << ":" << port << endl;

    int base = 0;
    int nextFrame = 0;
    int windowSize = 4;  // Initial window size
    bool done = false;

    // For timeout handling
    struct timeval lastSendTime;
    gettimeofday(&lastSendTime, NULL);

    while (base < MAX_FRAMES && !done) {
        // Send frames within window
        while (nextFrame < base + windowSize && nextFrame < MAX_FRAMES) {
            sendFrame(sockfd, receiverAddr, nextFrame);
            nextFrame++;
            gettimeofday(&lastSendTime, NULL);  // Update last send time
        }

        // Try to receive an ACK
        int ackNo, bufferSize;
        if (receiveAck(sockfd, ackNo, bufferSize)) {
            if (ackNo >= base) {
                base = ackNo + 1;
                windowSize = bufferSize;  // adjust window dynamically
                cout << "[Sender] Sliding window. Base: " << base << ", New Window Size: " << windowSize << "\n";
                
                // Reset timeout after receiving an ACK
                gettimeofday(&lastSendTime, NULL);
            }
        }

        // Check for timeout
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        long elapsedMs = (currentTime.tv_sec - lastSendTime.tv_sec) * 1000 + 
                        (currentTime.tv_usec - lastSendTime.tv_usec) / 1000;
                        
        if (elapsedMs > TIMEOUT_MS && base < nextFrame) {
            cout << "[Sender] Timeout occurred. Resending from frame " << base << endl;
            nextFrame = base;  // Reset next frame to send from base
            gettimeofday(&lastSendTime, NULL);  // Reset timeout
        }

        usleep(100000);  // Sleep for 100ms to avoid burning CPU
    }

    cout << "[Sender] All frames transmitted successfully!" << endl;
    
    // Send exit signal before closing
    sendExit(sockfd, receiverAddr);
    
    // Wait a bit for the exit signal to be processed
    sleep(1);
    
    close(sockfd);
    return 0;
}
