#include <chrono>
#include <winsock2.h>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
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
    serverAddress.sin_port = htons(6900); // Port number
    serverAddress.sin_addr.S_un.S_addr = inet_addr("172.24.198.250"); // IP address

    if (connect(sock, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    //Send identifier as client
    std::string client = "client";
    send(sock, client.c_str(), client.size(),  0);

    //User input
    std::string temp;
    char buffer[1024] = {0};
    while (std::getline(std::cin, temp))
    {
        if (temp != "Exit") {
            std::cout << "Enter number of hydrogen: ";
            std::string num_Hydrogen;
            std::getline(std::cin, num_Hydrogen);

            int H_max = std::stoi(num_Hydrogen);

            std::vector<std::string> hydrogen_List;
            std::string H_symbol = "H"; 
            for (int i = 1; i < H_max + 1; i++){
                std::string converted  = std::to_string(i);
                std::string combined = H_symbol.append(converted);
                hydrogen_List.push_back(combined);
                H_symbol = "H";
            }
            //send the size of the list 
            int hydrogenListSize = hydrogen_List.size();
            send(sock, reinterpret_cast<char*>(&hydrogenListSize), sizeof(hydrogenListSize), 0);

            // send the list itself
            for(const std::string& hydrogen : hydrogen_List) {
                send(sock, hydrogen.c_str(), hydrogen.size(), 0);
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
