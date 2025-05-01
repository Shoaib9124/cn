#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

using namespace std;

constexpr int BUFFER_SIZE = 1024;

int main() {
    // Initialize variables
    int client_socket;
    sockaddr_in server_addr{};
    vector<char> buffer(BUFFER_SIZE);
    string server_ip;
    int port;

    // Get server info from user
    cout << "Enter server IP: ";
    getline(cin, server_ip);

    cout << "Enter server port: ";
    cin >> port;
    cin.ignore(); // Clear the newline

    // Create the UDP socket
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }
    cout << "[+] UDP client socket created successfully.\n";

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        close(client_socket);
        return 1;
    }

    cout << "[+] Ready to communicate with server at " << server_ip << ":" << port << "\n";

    while (true) {
        // Get message from user
        cout << "Enter message ('exit' to quit): ";
        string message;
        getline(cin, message);

        if (sendto(client_socket, message.c_str(), message.size(), 0,
                   (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("Send failed");
            break;
        }

        if (message == "exit") {
            cout << "[-] Exiting client...\n";
            break;
        }

        // Wait for server response with timeout
        fd_set readfds;
        struct timeval timeout = {10, 0};  // 10 seconds timeout
        FD_ZERO(&readfds);
        FD_SET(client_socket, &readfds);

        if (select(client_socket + 1, &readfds, nullptr, nullptr, &timeout) > 0) {
            socklen_t addr_len = sizeof(server_addr);
		int recv_len = recvfrom(client_socket, buffer.data(), BUFFER_SIZE, 0,
                        (sockaddr*)&server_addr, &addr_len);

            if (recv_len < 0) {
                perror("Receive failed");
                continue;
            }

            string server_msg(buffer.data(), recv_len);
            cout << "[Server]: " << server_msg << "\n";

            if (server_msg == "exit") {
                cout << "[-] Server requested to end connection.\n";
                break;
            }
        } else {
            cout << "[-] Timeout: No response from server.\n";
        }
    }

    close(client_socket);
    cout << "[+] Client closed.\n";
    return 0;
}
