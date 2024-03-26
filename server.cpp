#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <stdlib.h>
#include <mutex>
#include <vector>
#include <algorithm>
#include <future>
#pragma comment(lib, "ws2_32.lib")
using namespace std;
const int PORT = 6900;
const int BUFFER_SIZE = 1024;
const char* SERVER_ADDRESS = "127.0.0.1";

mutex hydrogenArrayMutex;
mutex oxygenArrayMutex;

vector<SOCKET> hydrogenSockets;
vector<SOCKET> oxygenSockets;

vector<Request> hydrogenRequests;
vector<Request> oxygenRequests;

struct Request {
    string molecule_name;
    string timestamp;
};

void acceptClients(SOCKET serverSocket);

void handleClients(SOCKET clientSocket, char* type);

std::vector<std::pair<int,int>> getJobList(int start, int end, int numWorkers);

int main() {
    std::vector <int> primes;
    char buffer[BUFFER_SIZE] = {0};
    int start, end, numThreads;
    
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock\n";
        return -1;
    }

    // Create a socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket\n";
        WSACleanup();
        return -1;
    }

    // Configure server address
    sockaddr_in serverAddr;
    if (inet_pton(AF_INET, SERVER_ADDRESS, &(serverAddr.sin_addr.S_un.S_addr)) <=  0) {
        std::cerr << "inet_pton failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return   1;
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        std::cerr << "Error listening for connections\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Server IP Address: " << SERVER_ADDRESS << std::endl;

    std::cout << "Server listening on port " << PORT << std::endl;

    string str;

    std::thread acceptClientsThread(acceptClients, serverSocket);
    acceptClientsThread.detach();  // Detach the thread to allow it to run independently

    std::thread bondMoleculesThread(bondMolecules);
    bondMoleculesThread.detach();  // For handling bonding of molecules

    while(true) {
        std::cin >> str;
        std::cout << str;
    }

    // Close the server socket
    closesocket(serverSocket);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}

void acceptClients(SOCKET serverSocket) {
    char buffer[BUFFER_SIZE] = {0};
    while (true) {
        // Accept a client connection
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Error accepting connection\n";
            continue;
        }

        memset(buffer,  0, sizeof(buffer));
        int valread = recv(clientSocket, buffer, sizeof(buffer) -  1,  0);
        if (valread ==  0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        } else if (valread <  0) {
            std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            break;
        }

        // Handle the connection
        char* moleculeType;

        if(strcmp(buffer, "hydrogen") == 0){
            std::cout << "Accepted hydrogen connection from: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;
            moleculeType = "hydrogen";
            std::thread clientThread(handleClients, clientSocket, moleculeType);
            clientThread.detach();
            unique_lock<mutex> lock(hydrogenArrayMutex);
            hydrogenSockets.push_back(clientSocket);
            lock.unlock();
            // Send a message to the connected client
            send(clientSocket, SERVER_ADDRESS, strlen(SERVER_ADDRESS), 0);
        }else if(strcmp(buffer, "oxygen") == 0){
            std::cout << "Accepted oxygen connection from: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;
            moleculeType = "oxygen";
            std::thread clientThread(handleClients, clientSocket, moleculeType);
            clientThread.detach();
            unique_lock<mutex> lock(oxygenArrayMutex);
            oxygenSockets.push_back(clientSocket);
            lock.unlock();
            // Send a message to the connected client
            send(clientSocket, SERVER_ADDRESS, strlen(SERVER_ADDRESS), 0);
        }
    }
}

void handleClients(SOCKET clientSocket, char* type){
    char buffer[BUFFER_SIZE] = {0};
    while(true){
        // Receive the size of the primes vector
        recv(clientSocket, buffer, sizeof(buffer) -  1, 0);
        cout << buffer << endl;

        if(strcmp(type, "hydrogen")){

        } else if (strcmp(type, "oxygen")){

        }
    }
}

void bondMolecules(){
    while(true){

    }
}

