#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <iphlpapi.h>  // Для работы с интерфейсами
#include <csignal>
using namespace std;

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")  // Для использования IP Helper API

struct ClientInfo {
    SOCKET socket;
    string name;
};

vector<ClientInfo> clients;
SOCKET servSock; // Глобальный серверный сокет для обработки Ctrl+C

// Проверка, является ли IP-адрес локальным (192.168.x.x, 10.x.x.x, 172.16.x.x - 172.31.x.x)
bool is_private_ip(const string& ip) {
    unsigned int b1, b2, b3, b4;
    sscanf(ip.c_str(), "%u.%u.%u.%u", &b1, &b2, &b3, &b4);

    if ((b1 == 10) ||
        (b1 == 172 && b2 >= 16 && b2 <= 31) ||
        (b1 == 192 && b2 == 168)) {
        return true;  // Это локальный (приватный) IP
    }
    return false;
}

// Функция для получения локального IP-адреса
string get_local_ip() {
    char buffer[128] = { 0 };
    PIP_ADAPTER_INFO adapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    ULONG bufferSize = sizeof(IP_ADAPTER_INFO);

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
        adapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
    }

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapterInfo = adapterInfo;
        while (pAdapterInfo) {
            string ip = pAdapterInfo->IpAddressList.IpAddress.String;
            if (is_private_ip(ip) || ip == "127.0.0.1") {
                strcpy(buffer, ip.c_str());
                break;
            }
            pAdapterInfo = pAdapterInfo->Next;
        }
    }
    free(adapterInfo);
    return string(buffer);
}

// Закрытие сервера через Ctrl+C
void signalHandler(int signum) {
    cout << "\nInterrupt signal (Ctrl+C) received. Shutting down server...\n";
    for (auto& client : clients) {
        closesocket(client.socket);
    }
    closesocket(servSock);
    WSACleanup();
    exit(signum);
}

// Отправка сообщений другим клиентам
void receiveMessages(ClientInfo client) {
    char buffer[256] = { '\0' };
    int retVal;
    while (true) {
        retVal = recv(client.socket, buffer, sizeof(buffer), 0);
        if (retVal == SOCKET_ERROR) {
            cout << "Client " << client.name << " disconnected" << endl;
            closesocket(client.socket);
            clients.erase(remove_if(clients.begin(), clients.end(),
                [&](const ClientInfo& c) { return c.socket == client.socket; }), clients.end());
            return;
        }
        for (const auto& otherClient : clients) {
            if (otherClient.socket != client.socket) {
                string message = client.name + ": " + buffer;
                send(otherClient.socket, message.c_str(), message.size(), 0);
            }
        }
        cout << client.name << ": " << buffer << endl;
        memset(buffer, 0, sizeof(buffer));
    }
}

int main(void) {
    signal(SIGINT, signalHandler);  // Обработка Ctrl+C
    int retVal = 0;
    WORD sockVer;
    WSADATA wsaData;
    sockVer = MAKEWORD(2, 2);
    WSAStartup(sockVer, &wsaData);

    int port;
    cout << "Enter port to listen on: ";
    cin >> port;
    cin.ignore();

    servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (servSock == INVALID_SOCKET) {
        cout << "Unable to create socket\n";
        WSACleanup();
        return SOCKET_ERROR;
    }

    SOCKADDR_IN sin;
    sin.sin_family = PF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;

    retVal = bind(servSock, (LPSOCKADDR)&sin, sizeof(sin));
    if (retVal == SOCKET_ERROR) {
        cout << "Unable to bind\n";
        WSACleanup();
        return SOCKET_ERROR;
    }

    string local_ip = get_local_ip();
    if (local_ip.empty()) {
        cout << "Unable to determine local IP address\n";
        closesocket(servSock);
        WSACleanup();
        return 1;
    }
    cout << "Server started at " << local_ip << ", port " << port << endl;

    while (true) {
        if (listen(servSock, 10) == SOCKET_ERROR) {
            cout << "Unable to listen\n";
            WSACleanup();
            return SOCKET_ERROR;
        }

        SOCKADDR_IN clientAddr;
        int addrSize = sizeof(clientAddr);
        SOCKET clientSock = accept(servSock, (sockaddr*)&clientAddr, &addrSize);
        if (clientSock == INVALID_SOCKET) {
            cout << "Error connecting\n";
            continue;
        }

        cout << "New connection from " << inet_ntoa(clientAddr.sin_addr) << ", port " << ntohs(clientAddr.sin_port) << endl;
        char nameBuffer[256] = { '\0' };
        retVal = recv(clientSock, nameBuffer, sizeof(nameBuffer), 0);
        if (retVal == SOCKET_ERROR) {
            cout << "Error receiving name\n";
            closesocket(clientSock);
            continue;
        }

        string clientName(nameBuffer);
        cout << "Client '" << clientName << "' connected" << endl;

        for (const auto& otherClient : clients) {
            string message = clientName + " connected from " + inet_ntoa(clientAddr.sin_addr) + ", port " + to_string(ntohs(clientAddr.sin_port));
            send(otherClient.socket, message.c_str(), message.size(), 0);
        }

        clients.push_back({ clientSock, clientName });
        thread(receiveMessages, clients.back()).detach();
    }

    closesocket(servSock);
    WSACleanup();
    return 0;
}
