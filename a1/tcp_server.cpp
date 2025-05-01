#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

constexpr int BUFFER_SIZE = 1024;

int main() {
    int server_socket, client_socket;
    sockaddr_in server_addr{}, client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int port;

    cout << "Enter port number to listen on: ";
    cin >> port;
    cin.ignore(); // Clear newline

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }
    cout << "[+] TCP Server socket created successfully.\n";

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        return 1;
    }
    cout << "[+] Bound to port " << port << ".\n";

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        close(server_socket);
        return 1;
    }
    cout << "[+] Listening for connections...\n";

    client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
    if (client_socket < 0) {
        perror("Accept failed");
        close(server_socket);
        return 1;
    }

    cout << "[+] Client connected from "
         << inet_ntoa(client_addr.sin_addr) << ":"
         << ntohs(client_addr.sin_port) << "\n";

    vector<char> buffer(BUFFER_SIZE);

    while (true) {
        fill(buffer.begin(), buffer.end(), 0); // Optional

        int bytes_received = recv(client_socket, buffer.data(), buffer.size(), 0);
        if (bytes_received <= 0) {
            cout << "[-] Client disconnected or receive error.\n";
            break;
        }

        string client_msg(buffer.data(), bytes_received);
        cout << "[Client]: " << client_msg << "\n";

        if (client_msg == "exit") {
            cout << "[-] Client requested to close the connection.\n";
            break;
        }

        cout << "Enter message to client: ";
        string reply;
        getline(cin, reply);

        if (send(client_socket, reply.c_str(), reply.size(), 0) < 0) {
            perror("Send failed");
            break;
        }

        if (reply == "exit") {
            cout << "[-] Server exiting as requested.\n";
            break;
        }
    }

    close(client_socket);
    close(server_socket);
    cout << "[+] Connection closed.\n";
    return 0;
}
