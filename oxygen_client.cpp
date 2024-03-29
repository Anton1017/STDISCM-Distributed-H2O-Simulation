#include <chrono>
#include <winsock2.h>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <ctime>
#include <thread>
#include <mutex>

const int PORT = 6900;
const int BUFFER_SIZE = 1024;
const char* SERVER_ADDRESS = "127.0.0.1";
std::mutex socketMutex;
std::string getCurrentDate() {
    // Get the current time
    auto now = std::chrono::system_clock::now();
    
    // Convert the current time to a time_t object
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    // Convert the time_t object to a tm struct representing the current time
    std::tm* timeInfo = std::localtime(&currentTime);
    
    // Format the date
    char buffer[20]; // Buffer to store the formatted date
    std::strftime(buffer, 20, "%Y-%m-%d", timeInfo);
    
    // Convert the buffer to a string
    return std::string(buffer);
}

std::string getCurrentTime() {
    // Get the current time
    auto now = std::chrono::system_clock::now();
    
    // Convert the current time to a time_t object
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    // Convert the time_t object to a tm struct representing the current time
    std::tm* timeInfo = std::localtime(&currentTime);
    
    // Format the time
    char buffer[9]; // Buffer to store the formatted time (HH:MM:SS)
    std::strftime(buffer, 9, "%H:%M:%S", timeInfo);
    
    // Convert the buffer to a string
    return std::string(buffer);
}

void receiveLogs(SOCKET sock);

int main() {

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2,  2), &wsaData);
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
    serverAddress.sin_port = htons(PORT); // Port number
    serverAddress.sin_addr.S_un.S_addr = inet_addr(SERVER_ADDRESS); // IP address

    if (connect(sock, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    //Send identifier as oxygen client 
    std::string identifier = "oxygen";
    send(sock, identifier.c_str(), identifier.size(),  0);
    
    std::thread receiveLogsthread(receiveLogs, sock);
    receiveLogsthread.detach(); 

    //User input
    std::string temp;
    char buffer[1024] = {0};
    while (std::getline(std::cin, temp))
    {
        if (temp != "Exit") {
            std::cout << "Enter number of oxygen: ";
            std::string num_Oxygen;
            std::getline(std::cin, num_Oxygen);

            int O_max = std::stoi(num_Oxygen);

            std::vector<std::string> oxygen_List;
            std::string O_symbol = "O"; 
            for (int i = 1; i < O_max + 1; i++){
                std::string converted  = std::to_string(i);
                std::string combined = O_symbol.append(converted);
                oxygen_List.push_back(combined);
                O_symbol = "O";
            }
            
            //Send the size of the list
            int oxygenListSize = oxygen_List.size();
            //send(sock, reinterpret_cast<char*>(&oxygenListSize), sizeof(oxygenListSize), 0);
            //Send the list itself
            for(const std::string& oxygen : oxygen_List) {
                std::string currTime = getCurrentTime();
                std::string currDate = getCurrentDate();
                std::string log = oxygen + ", request, " +  currDate + " " + currTime;
                std::cout << log << std::endl;
                std::lock_guard<std::mutex> lock(socketMutex);
                send(sock, log.c_str(), strlen(log.c_str()), 0);
            }

            // std::cout << "Enter end point: ";
            // std::string endPoint;
            // std::getline(std::cin, endPoint);

            // std::cout << "Enter number of threads: ";
            // std::string numThreads;
            // std::getline(std::cin, numThreads);
            // std::string message = startPoint + "," + endPoint + "," + numThreads;

            //start timer
            // auto start = std::chrono::steady_clock::now();
            // send(sock, message.c_str(), message.size(),  0);


            //end timer
            // auto end = std::chrono::steady_clock::now();
            // std::chrono::duration<double> elapsed_seconds = end - start;
            // std::cout << "Time elapsed: " << elapsed_seconds.count() << "s" << std::endl;

        }
        else {
            send(sock, temp.c_str(), temp.size(),  0);
            break;
        }

    }
    recv(sock, buffer,  1024,  0);
    std::cout << "Server: " << buffer << std::endl;

    closesocket(sock);
    WSACleanup();

    return 0;
}

void receiveLogs(SOCKET sock){
    while (true) {
        //std::cout << "Listening for server responses: " << std::endl;
        char buffer[1024] = {0};
        recv(sock, buffer, sizeof(buffer) -  1, 0);
        std::cout << buffer << std::endl;
    }
}