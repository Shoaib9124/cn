// raw_client.cpp
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

using namespace std;

int main() {
    int sock;
    struct sockaddr_in server_addr{};
    string ip_str;
    int port;

    // 1) Get user input for IP and port
    cout << "Enter destination IP: ";
    getline(cin, ip_str);

    cout << "Enter destination port: ";
    cin >> port;
    cin.ignore();  // To clear newline after cin

    // 2) Create UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // 3) Build server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip_str.c_str(), &server_addr.sin_addr) != 1) {
        cerr << "Invalid IP address.\n";
        close(sock);
        return 1;
    }

    // 4) Send messages
    cout << "Enter messages to send. Type \"exit\" to quit.\n";
    while (true) {
        string msg;
        cout << "Message: ";
        getline(cin, msg);

        if (msg.empty()) continue;

        ssize_t sent = sendto(sock, msg.c_str(), msg.length(), 0,
                              (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (sent < 0) {
            perror("sendto");
            break;
        }

        if (msg == "exit") break;
    }

    close(sock);
    return 0;
}
