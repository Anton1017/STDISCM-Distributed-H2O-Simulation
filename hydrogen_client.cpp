#include <chrono>
#include <winsock2.h>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <ctime>
#include <thread>
#include <unordered_set>

const int PORT = 6900;
const int BUFFER_SIZE = 1024;
const char* SERVER_ADDRESS = "127.0.0.1";

std::vector<int> requestedHydrogen; 
std::vector<int> bondedHydrogen; 

bool hasDuplicates(const std::vector<int>& vec) {
    std::unordered_set<int> seen;
    for (int num : vec) {
        if (seen.find(num) != seen.end()) {
            return true; // Duplicate found
        }
        seen.insert(num);
    }
    return false; // No duplicates found
}

bool checkOrderH(int bondedH, std::vector<int> requestedHydrogenList){
    for (int i = 0; i <requestedHydrogenList.size(); i++)
    {
        if (bondedH == requestedHydrogenList[i]){
            return true;
        }
    }
    return false;
}
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

std::chrono::steady_clock::time_point receiveLogs(SOCKET sock, int size){
    //auto start = std::chrono::steady_clock::now();
    int ctr = 0;
    int requestNumber = 0;
    bool rightOrder = true;
    while (true) {
        //std::cout << "Listening for server responses: " << std::endl;
        char buffer[1024] = {0};
        int bytesReceived = recv(sock, reinterpret_cast<char*>(&requestNumber), sizeof(requestNumber), 0); 
        if (bytesReceived <= 0) {
            std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
            break; // Handle error appropriately
        }
        requestNumber = ntohl(requestNumber);
        bondedHydrogen.push_back(requestNumber);
        if (checkOrderH(requestNumber, requestedHydrogen) == false){
            std::cout<<requestNumber<<" has yet to be requested"<<std::endl;
            rightOrder = false;
            break;
        }
        std::string currTime = getCurrentTime();
        std::string currDate = getCurrentDate();
        std::string timestamp = currDate + " " + currTime;
       
        ctr++;
        //std::cout << buffer << std::endl;
        std::string log = "H" + std::to_string(requestNumber) + ", bonded, " + timestamp; 

        std::cout << log << std::endl;
        //std::cout << ctr << std::endl;
       
    
        if (ctr == size) {
            if (rightOrder == true){
                std::cout<<"Sanity Check: Hydrogens are in the right order"<<std::endl;
            }
            if (hasDuplicates(requestedHydrogen) == false &&hasDuplicates(bondedHydrogen) == false){
                std::cout<<"Sanity Check: There are no duplicates for both requested and bonded hydrogen"<<std::endl;
            }
            else {
                if (hasDuplicates(requestedHydrogen) == true)
                {
                    std::cout<<"Sanity Check: There are duplicates for requested hydrogen"<<std::endl;
                }
                else if (hasDuplicates(bondedHydrogen) == true)
                {
                    std::cout<<"Sanity Check: There are dupclicates for bonded hydrogen"<<std::endl;
                }
            }
            break;
        }
    }
    //auto end = std::chrono::steady_clock::now();

    //std::chrono::duration<double> elapsed_seconds = end - start;

    return std::chrono::steady_clock::now();
}

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

    //Send identifier as hydrogen
    std::string identifier = "hydrogen";
    send(sock, identifier.c_str(), identifier.size(),  0); 

    //User input
    std::string temp;
    char buffer[1024] = {0};
    int requestmsg = 0;
    while (std::getline(std::cin, temp))
    {
        if (temp != "Exit") {
            std::cout << "Enter number of hydrogen: ";
            std::string num_Hydrogen;
            std::getline(std::cin, num_Hydrogen);

            int H_max = std::stoi(num_Hydrogen);

            //std::thread receiveLogsthread(receiveLogs, sock, H_max);
            //auto receiveAsync = std::async(receiveLogs, sock, H_max);
            //receiveLogsthread.detach();

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
            //send(sock, reinterpret_cast<char*>(&hydrogenListSize), sizeof(hydrogenListSize), 0);

            // send the list itself
            /*
            for(const std::string& hydrogen : hydrogen_List) {
                std::string currTime = getCurrentTime();
                std::string currDate = getCurrentDate();
                std::string log = hydrogen + ", request, " +  currDate + " " + currTime;
                std::cout << log << std::endl;
                send(sock, log.c_str(), strlen(log.c_str()), 0);
            }
            */
            //start timer
            auto start = std::chrono::steady_clock::now();
            std::chrono::steady_clock::time_point end;
            std::thread receiveThread([&] {end = receiveLogs(sock, H_max);});

            for (int i = 1; i <= hydrogenListSize; i++) {
                requestmsg = htonl(i);  // Convert to network byte order

                if (send(sock, (char*)&requestmsg, sizeof(requestmsg), 0) == SOCKET_ERROR) {
                    std::cerr << "Failed to send request: " << WSAGetLastError() << std::endl;
                    break;  
                }
                requestedHydrogen.push_back(i);
                std::string currTime = getCurrentTime();
                std::string currDate = getCurrentDate();
                std::string log = "H" + std::to_string(i) + ", request, " +  currDate + " " + currTime;
                std::cout << log << std::endl;
            }
            
            //end timer
            // auto end = std::chrono::steady_clock::now();
            // std::chrono::duration<double> elapsed_seconds = end - start;
            // std::cout << "Time elapsed: " << elapsed_seconds.count() << "s" << std::endl;
            //auto end = receiveAsync.get();
            receiveThread.join();
            std::chrono::duration<double> elapsed_seconds = end - start;

            std::cout << "Time elapsed: " << elapsed_seconds.count() << "s" << std::endl;
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