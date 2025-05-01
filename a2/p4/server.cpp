#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <random>

#define BUFFER_SIZE 1024
#define TOTAL_FRAMES 10
#define WINDOW_SIZE 4

using namespace std;

// For frame loss simulation
random_device rd;
mt19937 gen(rd());
uniform_real_distribution<> dis(0, 1);

bool simulateLoss(double probability = 0.2) {
    return dis(gen) < probability;
}

int main() {
    int sockfd;
    sockaddr_in serverAddr, clientAddr;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    
    // Ask for server port
    int serverPort;
    string portStr;
    
    cout << "Enter server port to listen on (default: 12345): ";
    getline(cin, portStr);
    if (portStr.empty()) {
        serverPort = 12345;
    } else {
        serverPort = stoi(portStr);
    }
    
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Binding failed. Port may be in use." << endl;
        close(sockfd);
        return -1;
    }
    
    cout << "Server listening on port " << serverPort << endl;
    
    int mode;
    cout << "Enter communication mode:\n1. Non-NACK\n2. NACK\n3. Piggybacked\n> ";
    cin >> mode;
    
    cout << "Enter packet loss probability (0.0-1.0, default 0.2): ";
    double lossProbability;
    string lossStr;
    cin.ignore();
    getline(cin, lossStr);
    lossProbability = lossStr.empty() ? 0.2 : stod(lossStr);
    
    bool received[TOTAL_FRAMES] = {false};
    int expectedFrame = 0;
    socklen_t len = sizeof(clientAddr);
    
    cout << "\n[Server] Ready to receive frames...\n" << endl;
    
    while (true) {
        char buffer[BUFFER_SIZE];
        int rec = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (sockaddr*)&clientAddr, &len);
        
        if (rec < 0) {
            perror("Error receiving data");
            continue;
        }
        
        buffer[rec] = '\0';
        
        // Check if it's an exit message
        if (strcmp(buffer, "EXIT") == 0) {
            cout << "[Server] Received exit signal. Shutting down." << endl;
            break;
        }
        
        int frameNum;
        if (sscanf(buffer, "Frame %d", &frameNum) == 1) {
            cout << "[Server] Received: " << buffer << endl;
            
            if (mode == 1) { // Non-NACK mode
                if (frameNum >= 0 && frameNum < TOTAL_FRAMES) {
                    if (!simulateLoss(lossProbability)) {
                        char ack[BUFFER_SIZE];
                        snprintf(ack, sizeof(ack), "ACK %d", frameNum);
                        sendto(sockfd, ack, strlen(ack), 0, (sockaddr*)&clientAddr, len);
                        cout << "[Server] Sent: " << ack << endl;
                        received[frameNum] = true;
                    } else {
                        cout << "[Server] Simulated ACK loss for frame " << frameNum << endl;
                    }
                }
            } 
            else if (mode == 2) { // NACK mode
                if (frameNum == expectedFrame) {
                    received[frameNum] = true;
                    
                    if (!simulateLoss(lossProbability)) {
                        char ack[BUFFER_SIZE];
                        snprintf(ack, sizeof(ack), "ACK %d", frameNum);
                        sendto(sockfd, ack, strlen(ack), 0, (sockaddr*)&clientAddr, len);
                        cout << "[Server] Sent: " << ack << endl;
                    } else {
                        cout << "[Server] Simulated ACK loss for frame " << frameNum << endl;
                    }
                    
                    expectedFrame++;
                    while (expectedFrame < TOTAL_FRAMES && received[expectedFrame]) {
                        expectedFrame++;
                    }
                }
                else if (frameNum > expectedFrame) {
                    // Out of order frame received
                    received[frameNum] = true;
                    
                    if (!simulateLoss(lossProbability)) {
                        // Send NACK for the expected frame
                        char nack[BUFFER_SIZE];
                        snprintf(nack, sizeof(nack), "NACK %d", expectedFrame);
                        sendto(sockfd, nack, strlen(nack), 0, (sockaddr*)&clientAddr, len);
                        cout << "[Server] Sent: " << nack << " (out of order frame received)" << endl;
                    }
                }
                else if (frameNum < expectedFrame) {
                    // Duplicate frame - just ACK it
                    if (!simulateLoss(lossProbability)) {
                        char ack[BUFFER_SIZE];
                        snprintf(ack, sizeof(ack), "ACK %d", frameNum);
                        sendto(sockfd, ack, strlen(ack), 0, (sockaddr*)&clientAddr, len);
                        cout << "[Server] Sent: " << ack << " (duplicate frame)" << endl;
                    }
                }
            }
            else if (mode == 3) { // Piggybacked mode
                if (frameNum >= 0 && frameNum < TOTAL_FRAMES) {
                    received[frameNum] = true;
                    
                    if (!simulateLoss(lossProbability)) {
                        // In a real implementation, we would attach this ACK to the next outgoing data frame
                        char piggyback[BUFFER_SIZE];
                        snprintf(piggyback, sizeof(piggyback), "ACK %d (Piggybacked)", frameNum);
                        sendto(sockfd, piggyback, strlen(piggyback), 0, (sockaddr*)&clientAddr, len);
                        cout << "[Server] Sent: " << piggyback << endl;
                    } else {
                        cout << "[Server] Simulated piggybacked ACK loss for frame " << frameNum << endl;
                    }
                }
            }
        }
        
        // Check if all frames have been received
        bool allReceived = true;
        for (int i = 0; i < TOTAL_FRAMES; i++) {
            if (!received[i]) {
                allReceived = false;
                break;
            }
        }
        
        if (allReceived) {
            cout << "\n[Server] All " << TOTAL_FRAMES << " frames received successfully!" << endl;
        }
        
        usleep(100000); // 100ms delay
    }
    
    close(sockfd);
    return 0;
}
