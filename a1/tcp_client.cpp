#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

constexpr int BUFFER_SIZE = 1024;

int main() {
    int client_socket;
    sockaddr_in server_addr{};
    vector<char> buffer(BUFFER_SIZE);
    string server_ip;
    int port;

    cout << "Enter server IP address: ";
    getline(cin, server_ip);

    cout << "Enter server port number: ";
    cin >> port;
    cin.ignore(); // Clear newline

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }
    cout << "[+] TCP Client socket created successfully.\n";

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(client_socket);
        return 1;
    }

    cout << "Connecting to " << server_ip << ":" << port << "...\n";
    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        return 1;
    }

    cout << "[+] Connected to server.\n";

    while (true) {
        cout << "Enter message to server (type 'exit' to quit): ";
        string message;
        getline(cin, message);

        if (send(client_socket, message.c_str(), message.size(), 0) < 0) {
            perror("Send failed");
            break;
        }

        if (message == "exit") {
            cout << "[-] Client exiting...\n";
            break;
        }

        fill(buffer.begin(), buffer.end(), 0);

        int bytes_received = recv(client_socket, buffer.data(), buffer.size(), 0);
        if (bytes_received <= 0) {
            cout << "[-] Server disconnected or receive error.\n";
            break;
        }

        string server_msg(buffer.data(), bytes_received);
        cout << "[Server]: " << server_msg << "\n";

        if (server_msg == "exit") {
            cout << "[-] Server requested to close the connection.\n";
            break;
        }
    }

    close(client_socket);
    cout << "[+] Connection closed.\n";
    return 0;
}
