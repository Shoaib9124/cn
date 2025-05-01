// UDP Server Program in C++
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

using namespace std;

int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    vector<char> buffer(BUFFER_SIZE);
    string server_message;
    int port;
    
    // Get port number from user
    cout << "Enter port number to listen on: ";
    cin >> port;
    cin.ignore(); // Clear newline from input buffer
    
    // Create UDP socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        cerr << "Error creating socket: " << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }
    cout << "[+] UDP Server socket created successfully." << endl;
    
    // Prepare server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind socket to specified IP and port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Bind failed: " << strerror(errno) << endl;
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    cout << "[+] Bind to port " << port << " completed." << endl;
    cout << "[+] Server listening on port " << port << "..." << endl;
    
    // Communication loop
    while (true) {
        // Clear buffers
        fill(buffer.begin(), buffer.end(), 0);
        server_message.clear();
        memset(&client_addr, 0, client_len);
        
        cout << "Waiting for incoming messages..." << endl;
        
        // Receive message from client
        int recv_len = recvfrom(server_socket, buffer.data(), buffer.size(), 0,
                              (struct sockaddr*)&client_addr, &client_len);
        
        if (recv_len < 0) {
            cerr << "Error receiving data: " << strerror(errno) << endl;
            continue;
        }
        
        // Ensure buffer is null-terminated for string operations
        buffer[recv_len] = '\0';
        
        cout << "[Client " << inet_ntoa(client_addr.sin_addr) << ":" 
             << ntohs(client_addr.sin_port) << "]: " 
             << buffer.data() << endl;
        
        // Check for exit command
        if (strcmp(buffer.data(), "exit") == 0) {
            cout << "[-] Server exiting as requested by client." << endl;
            break;
        }
        
        // Get server's response
        cout << "Enter message to client: ";
        getline(cin, server_message);
        
        // Send response to client
        if (sendto(server_socket, server_message.c_str(), server_message.length(), 0,
                 (struct sockaddr*)&client_addr, client_len) < 0) {
            cerr << "Send failed: " << strerror(errno) << endl;
            continue;
        }
        
        // Check if server wants to exit
        if (server_message == "exit") {
            cout << "[-] Server exiting as requested." << endl;
            break;
        }
    }
    
    // Close socket
    close(server_socket);
    cout << "[+] Server closed." << endl;
    
    return 0;
}
