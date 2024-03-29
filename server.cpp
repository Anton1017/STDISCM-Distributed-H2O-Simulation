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

struct Request {
    string molecule_name;
    string timestamp;
    SOCKET clientSocket;
    bool isBonded = false;
};

mutex hydrogenArrayMutex;
mutex oxygenArrayMutex;
std::mutex socketMutex;
vector<SOCKET> hydrogenSockets;
vector<SOCKET> oxygenSockets;

vector<Request> hydrogenRequests;
vector<Request> oxygenRequests;

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

string createLog(Request req) {
    string log;
    
    if (req.isBonded) {
        log = req.molecule_name + ", bonded, " + getCurrentDate() + getCurrentTime();
    }
    else {
        log = req.molecule_name + ", request, " + getCurrentDate() + getCurrentTime();
    }

    return log;
}

void acceptClients(SOCKET serverSocket);

void handleClients(SOCKET clientSocket, char* type);
void handleHydrogenClient(SOCKET clientSocket);
void handleOxygenClient(SOCKET clientSocket);

std::vector<std::pair<int,int>> getJobList(int start, int end, int numWorkers);

void bondMolecules();

class Semaphore {
private:
    std::mutex mutex_;
    std::condition_variable condition_;
    size_t count_;

public:
    Semaphore(size_t count = 0) : count_(count) {}

    void notify() {
        std::unique_lock<std::mutex> lock(mutex_);
        ++count_;
        condition_.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        while(count_ == 0) {
            condition_.wait(lock);
        }
        --count_;
    }

    bool try_wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        if(count_ > 0) {
            --count_;
            return true;
        }
        return false;
    }
};

Semaphore H_semaphore = Semaphore(0);
Semaphore O_semaphore = Semaphore(0);

int main() {
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
            std::thread hydrogenClientThread(handleHydrogenClient, clientSocket);
            hydrogenClientThread.detach();
            hydrogenSockets.push_back(clientSocket);

            // Send a message to the connected client
            send(clientSocket, SERVER_ADDRESS, strlen(SERVER_ADDRESS), 0);
        } else if(strcmp(buffer, "oxygen") == 0){
            std::cout << "Accepted oxygen connection from: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;
            moleculeType = "oxygen";
            std::thread oxygenClientThread(handleOxygenClient, clientSocket);
            oxygenClientThread.detach();
            oxygenSockets.push_back(clientSocket);
            
            // Send a message to the connected client
            send(clientSocket, SERVER_ADDRESS, strlen(SERVER_ADDRESS), 0);
        }

    }
}

void handleClients(SOCKET clientSocket, char* type){
    char buffer[BUFFER_SIZE] = {0};

    while(true){
        
        recv(clientSocket, buffer, sizeof(buffer) -  1, 0);
        cout <<"Recieved Message"<< endl;
        
        
        cout << buffer << endl;

        // Is a hydrogen thread
        if (strcmp(type, "hydrogen") == 0 ){
            string request = buffer;
            string molecule_name = request.substr(0, request.find(","));
            string timestamp = request.substr(3, request.size() - 1);
            Request req = {molecule_name, timestamp, clientSocket};

            std::unique_lock<mutex> HArrayLock(hydrogenArrayMutex);
            cout << "I'm holding the hydrogen mutex now...\n";
            hydrogenRequests.push_back(req);
            
            cout << "I'm not holding the hydrogen mutex anymore...\n";

            if (hydrogenRequests.size() >= 2){
                H_semaphore.notify();
                //H_semaphore.notify();
            }
            HArrayLock.unlock();
        } 

        // Is an oxygen thread
        else if (strcmp(type, "oxygen") == 0){
            string request = buffer;
            string molecule_name = request.substr(0, request.find(","));
            string timestamp = request.substr(3, request.size() - 1);
            Request req = {molecule_name, timestamp, clientSocket};
            
            std::unique_lock<mutex> OArrayLock(oxygenArrayMutex);
            cout << "I'm holding the oxygen mutex now...\n";
            oxygenRequests.push_back(req);
            
            cout << "I'm not holding the oxygen mutex anymore...\n";

            if (oxygenRequests.size() >= 1){
                O_semaphore.notify();
            }
            OArrayLock.unlock();
        }   
    }
}

void handleHydrogenClient(SOCKET clientSocket){
    char buffer[BUFFER_SIZE] = {0};

    while(true){
        std::unique_lock<std::mutex> lock(socketMutex);
        recv(clientSocket, buffer, sizeof(buffer) -  1, 0);
        std::cout << "just received a new request\n";
        std::cout << buffer << endl;
        lock.unlock();
        string request = buffer;
        string molecule_name = request.substr(0, request.find(","));
        string timestamp = request.substr(3, request.size() - 1);
        Request req = {molecule_name, timestamp, clientSocket};
        std::cout << "Received new hydrogen request!\n" << endl;
        std::unique_lock<mutex> HArrayLock(hydrogenArrayMutex);
        std::cout << "I'm holding the hydrogen mutex now...\n";
        hydrogenRequests.push_back(req);
        
         std::cout << "I'm not holding the hydrogen mutex anymore...\n";

         if (hydrogenRequests.size() >= 2){
             H_semaphore.notify();
             H_semaphore.notify();
         }
         HArrayLock.unlock();
        
    }
}

void handleOxygenClient(SOCKET clientSocket){
    char buffer[BUFFER_SIZE] = {0};

    while(true){
        
        std::unique_lock<std::mutex> lock(socketMutex);
        recv(clientSocket, buffer, sizeof(buffer) -  1, 0);
        

        std::cout << buffer << endl;
        lock.unlock();
         string request = buffer;
         string molecule_name = request.substr(0, request.find(","));
         string timestamp = request.substr(3, request.size() - 1);
         Request req = {molecule_name, timestamp, clientSocket};
         std::cout << "Received new oxygen request!\n" << endl;
        
         std::unique_lock<mutex> OArrayLock(oxygenArrayMutex);
         std::cout << "I'm holding the oxygen mutex now...\n";
         oxygenRequests.push_back(req);
         std::cout << "I'm not holding the oxygen mutex anymore...\n";

         if (oxygenRequests.size() >= 1){
             O_semaphore.notify();
         }
         OArrayLock.unlock();
    }  
    
}

void bondMolecules() {
    while(true){
        H_semaphore.wait();
        cout << "Waiting for first H...\n";
        H_semaphore.wait();
        cout << "Waiting for second H...\n";
        O_semaphore.wait();
        cout << "Waiting for an O...\n";

        std::unique_lock<mutex> HArrayLock(hydrogenArrayMutex);
        Request H1 = hydrogenRequests[0];
        Request H2 = hydrogenRequests[1];
        hydrogenRequests.erase(hydrogenRequests.begin(), hydrogenRequests.begin() + 2);
        HArrayLock.unlock();

        std::unique_lock<mutex> OArrayLock(oxygenArrayMutex);
        Request O = oxygenRequests[0];
        oxygenRequests.erase(oxygenRequests.begin());
        OArrayLock.unlock();

        //cout << "Bonded: " << H1.molecule_name << " " << H2.molecule_name << " " << O.molecule_name << endl;
        // Make the logs for bonded atoms
        string log;
        H1.isBonded = true;
        H2.isBonded = true;
        O.isBonded = true;
        log = createLog(H1);
        cout << log << endl;
        send(H1.clientSocket, log.c_str(), log.size(), 0);

        log = createLog(H2);
        cout << log << endl;
        send(H2.clientSocket, log.c_str(), log.size(), 0);

        log = createLog(O);
        cout << log << endl;
        send(O.clientSocket, log.c_str(), log.size(), 0);
    }
}



