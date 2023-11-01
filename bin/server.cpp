#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <string>
#include <map>

struct ClientConnection {
    int socket;
    struct sockaddr_in clientAddr;
    std::string name;
};

class ClientManager {
public:
    ClientManager() : connectedClients(0) {}

    void addClient(int socket, const struct sockaddr_in& clientAddr, const std::string& name) {
        std::lock_guard<std::mutex> lock(mtx);
        clients.push_back({socket, clientAddr, name});
        connectedClients++;
    }

    void removeClient(int socket, const std::string& name) {
        std::lock_guard<std::mutex> lock(mtx);
        clients.erase(std::remove_if(clients.begin(), clients.end(), [socket](const ClientConnection& client) {
            return client.socket == socket;
        }), clients.end());
        connectedClients--;
        std::cout << "Client '" << name << "' disconnected" << std::endl;
    }

    int getConnectedClients() {
        std::lock_guard<std::mutex> lock(countMutex);
        return connectedClients;
    }

    std::vector<ClientConnection> getClients() {
        std::lock_guard<std::mutex> lock(mtx);
        return clients;
    }

private:
    std::vector<ClientConnection> clients;
    std::mutex mtx;
    int connectedClients;
    std::mutex countMutex;
};

ClientManager clientManager;

void handleClient(int clientSocket, const std::string& clientName) {
    clientManager.addClient(clientSocket, {}, clientName);

    char buffer[1024];

    while (true) {
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            close(clientSocket);
            clientManager.removeClient(clientSocket, clientName);
            break;
        }

        std::string message(buffer, bytesRead);

        if (message == "count") {
            std::string countMessage = "Number of users on the server: " + std::to_string(clientManager.getConnectedClients());
            ssize_t sentBytes = send(clientSocket, countMessage.c_str(), countMessage.size(), 0);
        } else {
            std::map<char, int> charCount;
            for (int i = 0; i < bytesRead; i++) {
                char c = buffer[i];
                charCount[c]++;
            }

            std::string countMessage = "Character count table:\n";
            for (const auto& entry : charCount) {
                countMessage += entry.first;
                countMessage += " ";
                countMessage += std::to_string(entry.second);
                countMessage += "\n";
            }

            ssize_t sentBytes = send(clientSocket, countMessage.c_str(), countMessage.size(), 0);

            for (const auto& client : clientManager.getClients()) {
                if (client.socket != clientSocket) {
                    ssize_t sentBytes = send(client.socket, (clientName + ": " + message).c_str(), bytesRead + clientName.size() + 2, 0);
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int serverPort = std::stoi(argv[1]);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating server socket" << std::endl;
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(serverPort);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding server socket" << std::endl;
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Error listening on port " << serverPort << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Server is listening on port " << serverPort << std::endl;

    while (true) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket < 0) {
            std::cerr << "Error accepting a connection from a client" << std::endl;
            continue;
        }

        char nameBuffer[1024];
        int nameBytesRead = recv(clientSocket, nameBuffer, sizeof(nameBuffer), 0);
        if (nameBytesRead <= 0) {
            std::cerr << "Client did not send a name" << std::endl;
            close(clientSocket);
            continue;
        }

        nameBuffer[nameBytesRead] = '\0';
        std::string clientName(nameBuffer);

        std::cout << "Client '" << clientName << "' connected" << std::endl;

        std::thread clientThread(handleClient, clientSocket, clientName);
        clientThread.detach();
    }

    close(serverSocket);

    return 0;
}