#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cerrno>
#include <thread>

void receiveMessages(int clientSocket) {
    while (true) {
        char buffer[1024];
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead == -1) {
            std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
            break;
        }

        if (bytesRead <= 0) {
            std::cerr << "Server disconnected." << std::endl;
            break;
        }

        std::cout << "Server response: " << std::string(buffer, bytesRead) << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>" << std::endl;
        return 1;
    }

    const char* serverIP = argv[1];
    int serverPort = std::stoi(argv[2]);

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error creating client socket: " << strerror(errno) << std::endl;
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error connecting to the server: " << strerror(errno) << std::endl;
        close(clientSocket);
        return 1;
    }

    std::cout << "Connected to the server." << std::endl;

    std::thread receiver(receiveMessages, clientSocket);

    while (true) {
        std::string message;
        std::cout << "Enter a command: " << std::endl;
        std::getline(std::cin, message);

        if (message == "exit") {
            break;
        }

        ssize_t bytesSent = send(clientSocket, message.c_str(), message.size(), 0);
        if (bytesSent == -1) {
            std::cerr << "Error sending data: " << strerror(errno) << std::endl;
            break;
        }
    }

    close(clientSocket);
    std::cout << "Connection closed." << std::endl;

    receiver.join();

    return 0;
}