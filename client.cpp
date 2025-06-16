#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 1234
#define BUFFER_SIZE 1024

// Thread function for receiving messages from the server.
void receiveMessages(int sock) {
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (received <= 0) {
            std::cout << "Connection closed by server.\n";
            break;
        }
        std::cout << buffer;
    }
}

int main() {
    int sock;
    struct sockaddr_in serverAddr;
    
    // Create a socket.
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Error: Could not create socket.\n";
        return 1;
    }
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    // Connect to local host.
    if(inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address or Address not supported.\n";
        return 1;
    }
    
    // Connect to the server.
    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Connection Failed.\n";
        return 1;
    }
    
    // Start a thread to listen for messages from the server.
    std::thread recvThread(receiveMessages, sock);
    
    // Main thread reads user input and sends messages.
    std::string message;
    while (true) {
        std::getline(std::cin, message);
        message += "\n";
        send(sock, message.c_str(), message.length(), 0);
        if (message == "exit\n") {
            break;
        }
    }
    
    close(sock);
    recvThread.join();
    return 0;
}