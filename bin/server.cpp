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

struct ClientConnection {
    int socket;
    struct sockaddr_in clientAddr;
};

std::vector<ClientConnection> clients;
std::mutex mtx;

void handleClient(int clientSocket, const std::string& clientName) {
    char buffer[1024];

    while (true) {
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            // Клиент отключился
            close(clientSocket);
            std::cout << "Клиент '" << clientName << "' отключился" << std::endl;

            std::unique_lock<std::mutex> lock(mtx);
            clients.erase(std::remove_if(clients.begin(), clients.end(), [clientSocket](const ClientConnection& client) {
                return client.socket == clientSocket;
            }), clients.end());
            break;
        }

        std::string message(buffer, bytesRead);

        std::unique_lock<std::mutex> lock(mtx);
        for (const auto& client : clients) {
            if (client.socket != clientSocket) {
                ssize_t sentBytes = send(client.socket, (clientName + ": " + message).c_str(), bytesRead + clientName.size() + 2, 0);
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0] << " <порт>" << std::endl;
        return 1;
    }

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int serverPort = std::stoi(argv[1]);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Ошибка при создании серверного сокета" << std::endl;
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(serverPort);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Ошибка при привязке серверного сокета" << std::endl;
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Ошибка при прослушивании порта " << serverPort << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Сервер слушает порт " << serverPort << std::endl;

    while (true) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket < 0) {
            std::cerr << "Ошибка при принятии подключения клиента" << std::endl;
            continue;
        }

        std::cout << "Клиент подключен" << std::endl;

        std::thread clientThread(handleClient, clientSocket, clientName);
        clientThread.detach(); // Отделяем поток, чтобы избежать блокировки сервера

        std::unique_lock<std::mutex> lock(mtx);
        clients.push_back({clientSocket, clientAddr});
    }

    close(serverSocket);

    return 0;
}