#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <random>
#include <vector>

#define BUFFER_SIZE 1024
#define WINDOW_SIZE 4

using namespace std;

bool simulateLoss(double probability = 0.3) {
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) < probability;
}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t len = sizeof(clientAddr);
    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    int port;
    cout << "Enter port number to listen on (default 12345): ";
    string portStr;
    getline(cin, portStr);
    port = portStr.empty() ? 12345 : stoi(portStr);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return 1;
    }

    cout << "Server listening on port " << port << endl;
    cout << "Select mode:\n1. Non-NACK based\n2. NACK based\n3. Piggybacked\nChoice: ";
    int mode;
    cin >> mode;

    // Packet loss simulation probability
    double lossProbability;
    cout << "Enter packet loss probability (0.0-1.0, default 0.3): ";
    string lossStr;
    cin.ignore(); // Clear the newline from previous input
    getline(cin, lossStr);
    lossProbability = lossStr.empty() ? 0.3 : stod(lossStr);

    int expectedFrame = 0;
    vector<bool> received(100, false); // Track received frames
    
    cout << "Server ready. Waiting for frames..." << endl;

    while (true) {
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&clientAddr, &len);
        if (n < 0) {
            perror("Error receiving data");
            continue;
        }
        
        buffer[n] = '\0';
        cout << "[Server] Received: " << buffer << endl;

        if (strcmp(buffer, "exit") == 0) {
            cout << "[Server] Received exit command. Shutting down." << endl;
            break;
        }

        int frameNum;
        if (sscanf(buffer, "FRAME %d", &frameNum) == 1) {
            // Process based on the selected mode
            if (mode == 1) { // Non-NACK
                if (frameNum == expectedFrame) {
                    cout << "[Server] Frame " << frameNum << " received correctly" << endl;
                    
                    // Simulate ACK loss
                    if (!simulateLoss(lossProbability)) {
                        snprintf(buffer, sizeof(buffer), "ACK %d", frameNum);
                        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&clientAddr, len);
                        cout << "[Server] Sent: " << buffer << endl;
                        expectedFrame++;
                    } else {
                        cout << "[Server] Simulated ACK loss for frame: " << frameNum << endl;
                    }
                } else {
                    cout << "[Server] Duplicate or out-of-order frame: " << frameNum << " (expected " << expectedFrame << ")" << endl;
                }

            } else if (mode == 2) { // NACK-based
                if (frameNum == expectedFrame) {
                    cout << "[Server] Frame " << frameNum << " received correctly" << endl;
                    
                    if (!simulateLoss(lossProbability)) {
                        snprintf(buffer, sizeof(buffer), "ACK %d", frameNum);
                        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&clientAddr, len);
                        cout << "[Server] Sent: " << buffer << endl;
                        expectedFrame++;
                    } else {
                        cout << "[Server] Simulated ACK loss for frame: " << frameNum << endl;
                    }
                } else {
                    cout << "[Server] Out-of-order frame. Expected " << expectedFrame << ", got " << frameNum << endl;
                    
                    if (!simulateLoss(lossProbability)) {
                        snprintf(buffer, sizeof(buffer), "NACK %d", expectedFrame);
                        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&clientAddr, len);
                        cout << "[Server] Sent: " << buffer << endl;
                    } else {
                        cout << "[Server] Simulated NACK loss for frame: " << expectedFrame << endl;
                    }
                }

            } else if (mode == 3) { // Piggybacked
                cout << "[Server] Frame " << frameNum << " received" << endl;
                received[frameNum] = true;
                
                // Find next expected frame
                while (received[expectedFrame] && expectedFrame < 100) {
                    expectedFrame++;
                }
                
                // Send ACK with piggybacked data
                if (!simulateLoss(lossProbability)) {
                    snprintf(buffer, sizeof(buffer), "ACK %d", frameNum);
                    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&clientAddr, len);
                    cout << "[Server] Sent piggybacked: " << buffer << endl;
                } else {
                    cout << "[Server] Simulated ACK loss for frame: " << frameNum << endl;
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
