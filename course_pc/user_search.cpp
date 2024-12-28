#undef byte

#include <winsock2.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

SOCKET createClientSocket() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed.\n";
        exit(EXIT_FAILURE);
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed.\n";
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Connection to server failed.\n";
        closesocket(clientSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    return clientSocket;
}

void sendRequest(SOCKET clientSocket, const string& request) {
    if (send(clientSocket, request.c_str(), request.size(), 0) == SOCKET_ERROR) {
        cerr << "Send failed.\n";
        closesocket(clientSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
}

string receiveResponse(SOCKET clientSocket) {
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        cerr << "Receive failed or connection closed.\n";
        closesocket(clientSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    buffer[bytesReceived] = '\0';
    return string(buffer);
}

int main() {
    srand(time(0));

    vector<string> words = {"movie", "son", "sometimes", "woman", "fun", "Gismo"};
    string randomWord;
    cout << "Enter a search term or press enter to auto-select a word: ";
    getline(cin, randomWord);
    if(randomWord == "") {
        randomWord = words[rand() % words.size()];
        cout << "Searching for the word: " << randomWord << endl;
    }
    SOCKET clientSocket = createClientSocket();

    string request = "GET /search?word=" + randomWord + " HTTP/1.1\r\n" "Host: " + string(SERVER_IP) + "\r\n" "Connection: close\r\n\r\n";

    sendRequest(clientSocket, request);

    string response = receiveResponse(clientSocket);
    cout << "Server response:\n" << response << endl;

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
