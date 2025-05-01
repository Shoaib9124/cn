#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <random>
#include <fcntl.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024
#define TOTAL_FRAMES 10
#define WINDOW_SIZE 4
#define TIMEOUT_MS 3000

using namespace std;

random_device rd;
mt19937 gen(rd());
uniform_real_distribution<> dis(0, 1);

bool simulateLoss(double probability = 0.2) {
    return dis(gen) < probability;  // Fixed: changed > to < for consistency
}

void sendFrame(int sockfd, sockaddr_in &serverAddr, int frame) {
    if (!simulateLoss()) {
        char buffer[BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer), "Frame %d", frame);
        sendto(sockfd, buffer, strlen(buffer), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
        cout << "[Client] Sent: " << buffer << endl;
    } else {
        cout << "[Client] Frame " << frame << " lost during transmission (simulated loss)\n";
    }
}

int main() {
    int sockfd;
    sockaddr_in serverAddr;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    // Set socket to non-blocking for timeout implementation
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    
    // Ask for server IP and port
    string serverIP;
    int serverPort;
    
    cout << "Enter server IP address (default: 127.0.0.1): ";
    getline(cin, serverIP);
    if (serverIP.empty()) {
        serverIP = "127.0.0.1";
    }
    
    cout << "Enter server port (default: 12345): ";
    string portStr;
    getline(cin, portStr);
    if (portStr.empty()) {
        serverPort = 12345;
    } else {
        serverPort = stoi(portStr);
    }
    
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
    
    cout << "Connecting to " << serverIP << ":" << serverPort << endl;
    
    int mode;
    cout << "Enter communication mode:\n1. Non-NACK\n2. NACK\n3. Piggybacked\n> ";
    cin >> mode;
    
    int base = 0, nextFrame = 0;
    bool acked[TOTAL_FRAMES] = {false};
    
    // For timeout handling
    struct timeval lastSendTime;
    gettimeofday(&lastSendTime, NULL);
    
    cout << "\n[Client] Starting transmission with " << TOTAL_FRAMES << " frames...\n" << endl;
    
    while (base < TOTAL_FRAMES) {
        // Send frames within the window that haven't been sent yet
        while (nextFrame < base + WINDOW_SIZE && nextFrame < TOTAL_FRAMES) {
            sendFrame(sockfd, serverAddr, nextFrame);
            nextFrame++;
            gettimeofday(&lastSendTime, NULL); // Update send time
        }
        
        // Check for incoming ACKs
        char ack[BUFFER_SIZE];
        sockaddr_in temp;
        socklen_t len = sizeof(temp);
        int recvLen = recvfrom(sockfd, ack, BUFFER_SIZE, 0, (sockaddr*)&temp, &len);
        
        if (recvLen > 0) {
            ack[recvLen] = '\0';
            int ackNum;
            
            if (sscanf(ack, "ACK %d", &ackNum) == 1) {
                cout << "[Client] Received: " << ack << endl;
                
                if (ackNum >= 0 && ackNum < TOTAL_FRAMES) {
                    acked[ackNum] = true;
                    
                    // Advance base to the first unacknowledged frame
                    while (base < TOTAL_FRAMES && acked[base]) {
                        base++;
                    }
                    
                    cout << "[Client] Window base moved to: " << base << endl;
                }
            } else if (mode == 2 && strncmp(ack, "NACK", 4) == 0) {
                int nackNum;
                if (sscanf(ack + 5, "%d", &nackNum) == 1) {
                    cout << "[Client] Received NACK for frame: " << nackNum << endl;
                    
                    // Resend the NACK'd frame
                    if (nackNum >= base && nackNum < nextFrame) {
                        sendFrame(sockfd, serverAddr, nackNum);
                        gettimeofday(&lastSendTime, NULL);
                    }
                }
            }
        }
        
        // Check for timeout
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        long elapsedMs = (currentTime.tv_sec - lastSendTime.tv_sec) * 1000 + 
                        (currentTime.tv_usec - lastSendTime.tv_usec) / 1000;
                        
        if (elapsedMs > TIMEOUT_MS && base < nextFrame) {
            cout << "[Client] Timeout occurred. Resending unacknowledged frames starting from " << base << endl;
            
            // Resend all frames in the current window that haven't been acknowledged
            for (int i = base; i < nextFrame && i < base + WINDOW_SIZE; i++) {
                if (!acked[i]) {
                    sendFrame(sockfd, serverAddr, i);
                }
            }
            
            gettimeofday(&lastSendTime, NULL); // Reset timeout
        }
        
        usleep(100000); // 100ms delay for better responsiveness and CPU usage
    }
    
    cout << "\n[Client] All frames transmitted successfully!" << endl;
    
    // Send termination signal to server
    const char* exitMsg = "EXIT";
    sendto(sockfd, exitMsg, strlen(exitMsg), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
    cout << "[Client] Sent exit signal to server." << endl;
    
    close(sockfd);
    return 0;
}
