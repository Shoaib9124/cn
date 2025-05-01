#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include <random>
#include <fcntl.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024
#define WINDOW_SIZE 4
#define TOTAL_FRAMES 10
#define TIMEOUT_MS 2000

using namespace std;

bool simulateLoss(double probability = 0.1) {
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) < probability;
}

void sendFrame(int sockfd, struct sockaddr_in &serverAddr, int frameNum) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "FRAME %d", frameNum);
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    cout << "[Client] Sent: " << buffer << endl;
}

void sendAck(int sockfd, struct sockaddr_in &serverAddr, int ackNum) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "ACK %d", ackNum);
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    cout << "[Client] Sent: " << buffer << endl;
}

void sendNack(int sockfd, struct sockaddr_in &serverAddr, int nackNum) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "NACK %d", nackNum);
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    cout << "[Client] Sent: " << buffer << endl;
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

    // Set socket to non-blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    string ip;
    int port;
    cout << "Enter server IP (default 127.0.0.1): ";
    getline(cin, ip);
    if (ip.empty()) ip = "127.0.0.1";

    cout << "Enter server port (default 12345): ";
    string portStr;
    getline(cin, portStr);
    port = portStr.empty() ? 12345 : stoi(portStr);

    cout << "Select mode:\n1. Non-NACK\n2. NACK-based\n3. Piggybacked\nEnter choice: ";
    int mode;
    cin >> mode;

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

    int base = 0, nextFrame = 0;
    socklen_t addrLen = sizeof(serverAddr);
    bool acked[TOTAL_FRAMES] = {false};
    
    // For timeout handling
    struct timeval lastSendTime;
    gettimeofday(&lastSendTime, NULL);

    while (base < TOTAL_FRAMES) {
        // Send frames within the window that haven't been sent yet
        while (nextFrame < base + WINDOW_SIZE && nextFrame < TOTAL_FRAMES) {
            sendFrame(sockfd, serverAddr, nextFrame);
            nextFrame++;
            gettimeofday(&lastSendTime, NULL); // Update send time
        }

        // Process incoming messages
        ssize_t len = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&serverAddr, &addrLen);
        if (len > 0) {
            buffer[len] = '\0';

            if (strncmp(buffer, "ACK", 3) == 0) {
                int ackNum = atoi(buffer + 4);
                cout << "[Client] Received ACK for frame " << ackNum << endl;
                
                // Mark this frame as acknowledged
                if (ackNum >= 0 && ackNum < TOTAL_FRAMES) {
                    acked[ackNum] = true;
                }
                
                // Move base forward to the first unacknowledged frame
                while (base < TOTAL_FRAMES && acked[base]) {
                    base++;
                }
                
            } else if (strncmp(buffer, "NACK", 4) == 0 && mode == 2) {
                int nackNum = atoi(buffer + 5);
                cout << "[Client] Received NACK for frame " << nackNum << " - Resending from here" << endl;
                
                // Only reset nextFrame if it's a valid NACK
                if (nackNum >= base && nackNum < nextFrame) {
                    nextFrame = nackNum;
                    gettimeofday(&lastSendTime, NULL); // Reset timeout on NACK
                }
                
            } else if (strncmp(buffer, "FRAME", 5) == 0 && mode == 3) {
                int frameNum = atoi(buffer + 6);
                cout << "[Client] Received frame " << frameNum << " (Piggybacked)" << endl;
                sendAck(sockfd, serverAddr, frameNum);
            }
        }

        // Check for timeout
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        long elapsedMs = (currentTime.tv_sec - lastSendTime.tv_sec) * 1000 + 
                        (currentTime.tv_usec - lastSendTime.tv_usec) / 1000;
                        
        if (elapsedMs > TIMEOUT_MS && base < nextFrame) {
            cout << "[Client] Timeout occurred. Resending from frame " << base << endl;
            nextFrame = base;
            gettimeofday(&lastSendTime, NULL); // Reset timeout
        }

        this_thread::sleep_for(chrono::milliseconds(100));  // Shorter delay for responsiveness
    }

    cout << "[Client] Transmission complete." << endl;
    
    // Send termination signal to server
    const char* exitMsg = "exit";
    sendto(sockfd, exitMsg, strlen(exitMsg), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    cout << "[Client] Sent exit signal to server." << endl;
    
    close(sockfd);
    return 0;
}
