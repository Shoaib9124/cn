// raw_server.cpp
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define BUFFER_SIZE 65536

using namespace std;

int main() {
    int raw_sock;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int watch_port;

    // 1) Ask user which UDP port to sniff
    cout << "Enter port to sniff on: ";
    cin >> watch_port;
    cin.ignore(); // Remove newline
    cout << "[+] Listening for packets on port " << watch_port << "...\n";

    // 2) Create raw socket
    raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (raw_sock < 0) {
        perror("socket");
        cerr << "[-] Note: raw sockets require root privileges (use sudo).\n";
        return 1;
    }

    // 3) Start receive loop
    while (true) {
        ssize_t len = recvfrom(raw_sock, buffer, BUFFER_SIZE, 0,
                               (struct sockaddr*)&addr, &addr_len);
        if (len < 0) {
            perror("recvfrom");
            continue;
        }

        if (len < (int)(sizeof(iphdr) + sizeof(udphdr))) {
            cerr << "[-] Packet too short, skipping.\n";
            continue;
        }

        // 4) Extract IP header
        struct iphdr *ip = (struct iphdr*)buffer;
        int ip_header_len = ip->ihl * 4;

        if (len < ip_header_len + (int)sizeof(udphdr)) {
            cerr << "[-] Truncated packet.\n";
            continue;
        }

        // 5) Extract UDP header
        struct udphdr *udp = (struct udphdr*)(buffer + ip_header_len);
        int src_port = ntohs(udp->source);
        int dst_port = ntohs(udp->dest);

        if (dst_port != watch_port) {
            continue; // Ignore packets not destined for this port
        }

        // 6) Extract payload
        int udp_header_len = sizeof(struct udphdr);
        int data_len = len - ip_header_len - udp_header_len;
        char *data = buffer + ip_header_len + udp_header_len;

        // 7) Display packet info
        struct in_addr src_ip;
        src_ip.s_addr = ip->saddr;

        cout << "\n[Packet Captured]\n";
        cout << "  From IP   : " << inet_ntoa(src_ip) << endl;
        cout << "  From Port : " << src_port << endl;
        cout << "  To   Port : " << dst_port << endl;
        cout << "  Payload   : ";

        if (data_len > 0) {
            cout.write(data, data_len);
        } else {
            cout << "(No payload)";
        }

        cout << endl;
    }

    close(raw_sock);
    return 0;
}
