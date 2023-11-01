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

std::vector<ClientConnection> clients;
std::mutex mtx;
int connectedClients = 0;
std::mutex countMutex;

void handleClient(int clientSocket, const std::string& clientName) {
    {
        std::unique_lock<std::mutex> lock(countMutex);
        connectedClients++;
    }

    char buffer[1024];

    while (true) {
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            close(clientSocket);
            std::cout << "Клиент '" << clientName << "' отключился" << std::endl;

            {
                std::unique_lock<std::mutex> lock(countMutex);
                connectedClients--;
            }

            std::unique_lock<std::mutex> lock(mtx);
            clients.erase(std::remove_if(clients.begin(), clients.end(), [clientSocket](const ClientConnection& client) {
                return client.socket == clientSocket;
            }), clients.end());
            break;
        }

        std::string message(buffer, bytesRead);

        if (message == "count") {
            // Отправляем клиенту количество пользователей на сервере
            std::unique_lock<std::mutex> lock(countMutex);
            std::string countMessage = "Количество пользователей на сервере: " + std::to_string(connectedClients);
            ssize_t sentBytes = send(clientSocket, countMessage.c_str(), countMessage.size(), 0);
        } else {
            // Считаем количество различных символов в сообщении
            std::map<char, int> charCount;
            for (int i = 0; i < bytesRead; i++) {
                char c = buffer[i];
                charCount[c]++;
            }

            // Строим ответное сообщение с таблицей
            std::string countMessage = "Таблица количества символов:\n";
            for (const auto& entry : charCount) {
                countMessage += entry.first;
                countMessage += " ";
                countMessage += std::to_string(entry.second);
                countMessage += "\n";
            }

            // Отправляем ответ клиенту
            ssize_t sentBytes = send(clientSocket, countMessage.c_str(), countMessage.size(), 0);
            std::unique_lock<std::mutex> lock(mtx);
            for (const auto& client : clients) {
                if (client.socket != clientSocket) {
                    ssize_t sentBytes = send(client.socket, (clientName + ": " + message).c_str(), bytesRead + clientName.size() + 2, 0);
                }
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
        std::cerr << "Ошибка создания серверного сокета" << std::endl;
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(serverPort);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Ошибка привязки серверного сокета" << std::endl;
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
            std::cerr << "Ошибка при приеме соединения с клиентом" << std::endl;
            continue;
        }

        char nameBuffer[1024];
        int nameBytesRead = recv(clientSocket, nameBuffer, sizeof(nameBuffer), 0);
        if (nameBytesRead <= 0) {
            std::cerr << "Клиент не отправил имя" << std::endl;
            close(clientSocket);
            continue;
        }

        nameBuffer[nameBytesRead] = '\0';
        std::string clientName(nameBuffer);

        std::cout << "Клиент '" << clientName << "' подключился" << std::endl;

        std::thread clientThread(handleClient, clientSocket, clientName);
        clientThread.detach();

        std::unique_lock<std::mutex> lock(mtx);
        clients.push_back({clientSocket, clientAddr, clientName});
    }

    close(serverSocket);

    return 0;
}