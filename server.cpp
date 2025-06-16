#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 1234
#define BUFFER_SIZE 1024

// Thread-safe list of connected client sockets.
std::vector<int> clientSockets;
std::mutex clientsMutex;

// Hardcoded user database (username -> password)
std::map<std::string, std::string> users = {
    {"user1", "password1"},
    {"user2", "password2"}
};

// Broadcast a message to all clients except the sender.
void broadcastMessage(const std::string &message, int senderSocket) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (int sock : clientSockets) {
        if (sock != senderSocket) {
            send(sock, message.c_str(), message.length(), 0);
        }
    }
}

// Handle individual client session.
void clientHandler(int clientSocket) {
    char buffer[BUFFER_SIZE];
    std::string username;

    // Send username prompt with newline.
    std::string prompt = "Enter username:\n";
    send(clientSocket, prompt.c_str(), prompt.length(), 0);
    
    memset(buffer, 0, BUFFER_SIZE);
    int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
        close(clientSocket);
        return;
    }
    username = std::string(buffer);
    // Trim newline and carriage return characters.
    username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
    username.erase(std::remove(username.begin(), username.end(), '\r'), username.end());

    // Send password prompt with newline.
    prompt = "Enter password:\n";
    send(clientSocket, prompt.c_str(), prompt.length(), 0);
    
    memset(buffer, 0, BUFFER_SIZE);
    bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
        close(clientSocket);
        return;
    }
    std::string password(buffer);
    password.erase(std::remove(password.begin(), password.end(), '\n'), password.end());
    password.erase(std::remove(password.begin(), password.end(), '\r'), password.end());

    // Check credentials.
    if (users.find(username) == users.end() || users[username] != password) {
        std::string failureMsg = "Authentication Failed!\n";
        send(clientSocket, failureMsg.c_str(), failureMsg.length(), 0);
        close(clientSocket);
        return;
    } else {
        std::string successMsg = "Authentication Successful!\nWelcome to the chat server. Type your messages (type 'exit' to disconnect):\n";
        send(clientSocket, successMsg.c_str(), successMsg.length(), 0);
    }

    // Add client socket to list.
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clientSockets.push_back(clientSocket);
    }

    // Receive and broadcast messages.
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            break;
        }
        std::string msg(buffer);
        msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end());
        msg.erase(std::remove(msg.begin(), msg.end(), '\r'), msg.end());
        
        if (msg == "exit") {
            break;
        }
        
        std::string fullMessage = username + ": " + msg + "\n";
        std::cout << fullMessage;
        broadcastMessage(fullMessage, clientSocket);
    }
    
    // Remove client from list and close socket.
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
    }
    close(clientSocket);
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    // Create server socket.
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error: Could not create socket.\n";
        return 1;
    }
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Bind failed.\n";
        close(serverSocket);
        return 1;
    }
    
    // Listen for incoming connections.
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Error: Listen failed.\n";
        close(serverSocket);
        return 1;
    }
    
    std::cout << "Chat server started on port " << PORT << "\n";
    
    // Accept incoming client connections.
    while (true) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
        if (clientSocket < 0) {
            std::cerr << "Error: Accept failed.\n";
            continue;
        }
        std::thread(clientHandler, clientSocket).detach();
    }
    
    close(serverSocket);
    return 0;
}