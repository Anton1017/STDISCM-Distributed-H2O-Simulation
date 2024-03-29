#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <ctime>
#include <thread>
#include <mutex>
#include <chrono>

const int PORT = 6900;
const int BUFFER_SIZE = 1024;
const char* SERVER_ADDRESS = "127.0.0.1";

std::mutex socketMutex; // Mutex for protecting socket operations

std::string getCurrentDate() {
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm* timeInfo = std::localtime(&currentTime);
    char buffer[20];
    std::strftime(buffer, 20, "%Y-%m-%d", timeInfo);
    return std::string(buffer);
}

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm* timeInfo = std::localtime(&currentTime);
    char buffer[9];
    std::strftime(buffer, 9, "%H:%M:%S", timeInfo);
    return std::string(buffer);
}

void receiveLogs(SOCKET sock) {
    while (true) {
        char buffer[1024] = {0};
        int result = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (result > 0) {
            std::cout << buffer << std::endl;
        } else if (result == 0) {
            std::cout << "Connection closed by the server." << std::endl;
            break;
        } else {
            std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.S_un.S_addr = inet_addr(SERVER_ADDRESS);

    if (connect(sock, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::string identifier = "hydrogen";
    send(sock, identifier.c_str(), identifier.size(), 0);

    std::thread receiveLogsthread(receiveLogs, sock);
    receiveLogsthread.detach();

    std::string temp;
    char buffer[1024] = {0};
    while (std::getline(std::cin, temp)) {
        if (temp != "Exit") {
            std::cout << "Enter number of hydrogen: ";
            std::string num_Hydrogen;
            std::getline(std::cin, num_Hydrogen);

            int H_max = std::stoi(num_Hydrogen);
            std::vector<std::string> hydrogen_List;
            std::string H_symbol = "H";
            for (int i = 1; i < H_max + 1; i++) {
                std::string converted = std::to_string(i);
                std::string combined = H_symbol.append(converted);
                hydrogen_List.push_back(combined);
                H_symbol = "H";
            }

            for (const std::string& hydrogen : hydrogen_List) {
                std::string currTime = getCurrentTime();
                std::string currDate = getCurrentDate();
                std::string log = hydrogen + ", request, " + currDate + " " + currTime;
                std::cout << log << std::endl;
                std::unique_lock<std::mutex> lock(socketMutex);//ock the mutex for socket operations
                send(sock, log.c_str(), log.size(), 0);
                lock.unlock();
        
            }
        } else {
            
                send(sock, temp.c_str(), temp.size(), 0);
            
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(socketMutex); // Lock the mutex for socket operations
        recv(sock, buffer, 1024, 0);
    }
    std::cout << "Server: " << buffer << std::endl;

    closesocket(sock);
    WSACleanup();

    return 0;
}
