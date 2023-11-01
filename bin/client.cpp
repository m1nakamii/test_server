#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cerrno>
#include <thread>

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

    std::string userName;
    std::cout << "Enter your name: ";
    std::getline(std::cin, userName);

    ssize_t nameBytesSent = send(clientSocket, userName.c_str(), userName.size(), 0);
    if (nameBytesSent == -1) {
        std::cerr << "Error sending name: " << strerror(errno) << std::endl;
        close(clientSocket);
        return 1;
    }

    std::thread receiver([](int socket) {
    while (true) {
        char buffer[1024];
        ssize_t bytesRead = recv(socket, buffer, sizeof(buffer), 0);

        if (bytesRead == -1) {
            std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
            break;
        }

        if (bytesRead <= 0) {
            std::cerr << "Server disconnected." << std::endl;
            break;
        }

        std::cout << std::string(buffer, bytesRead) << std::endl;
    }
    }, clientSocket);

    while (true) {
        std::string message;
        std::getline(std::cin, message);

        if (message == "/exit") {
            break;
        } else if (message == "/count") {
            ssize_t bytesSent = send(clientSocket, message.c_str(), message.size(), 0);
            if (bytesSent == -1) {
                std::cerr << "Error sending data: " << strerror(errno) << std::endl;
                break;
            }
        } else {
            // Отправить обычное сообщение
            ssize_t bytesSent = send(clientSocket, message.c_str(), message.size(), 0);
            if (bytesSent == -1) {
                std::cerr << "Error sending data: " << strerror(errno) << std::endl;
                break;
            }
        }
    }

    close(clientSocket);
    std::cout << "Connection closed." << std::endl;

    receiver.join();
    return 0;
}