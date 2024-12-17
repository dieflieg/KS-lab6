#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iphlpapi.h>  // Для работы с интерфейсами

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")  // Для использования IP Helper API

using namespace std;

// Проверка, является ли IP-адрес локальным (192.168.x.x, 10.x.x.x, 172.16.x.x - 172.31.x.x)
bool is_private_ip(const string& ip) {
    unsigned int b1, b2, b3, b4;
    sscanf(ip.c_str(), "%u.%u.%u.%u", &b1, &b2, &b3, &b4);

    if ((b1 == 10) ||
        (b1 == 172 && b2 >= 16 && b2 <= 31) ||
        (b1 == 192 && b2 == 168)) {
        return true;
    }
    return false;
}

// Получение IP-адреса локального хоста
string get_local_ip() {
    PIP_ADAPTER_INFO adapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    ULONG bufferSize = sizeof(IP_ADAPTER_INFO);

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
        adapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
    }

    string ip = "";
    if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapterInfo = adapterInfo;
        while (pAdapterInfo) {
            string curr_ip = pAdapterInfo->IpAddressList.IpAddress.String;
            if (is_private_ip(curr_ip) || curr_ip == "127.0.0.1") {
                ip = curr_ip;
                break;
            }
            pAdapterInfo = pAdapterInfo->Next;
        }
    }
    free(adapterInfo);
    return ip.empty() ? "127.0.0.1" : ip;  // Возвращаем 127.0.0.1 по умолчанию
}

int main(void) {
    WORD sockVer = MAKEWORD(2, 2);
    WSADATA wsaData;
    WSAStartup(sockVer, &wsaData);

    int port;
    cout << "Enter the port number to listen on: ";
    cin >> port;

    // Создание сокета
    SOCKET servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (servSock == INVALID_SOCKET) {
        cout << "Unable to create socket" << endl;
        WSACleanup();
        return SOCKET_ERROR;
    }

    SOCKADDR_IN sin;
    sin.sin_family = PF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;  // Привязка к любому интерфейсу

    if (bind(servSock, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR) {
        cout << "Unable to bind" << endl;
        WSACleanup();
        return SOCKET_ERROR;
    }

    // Получение и вывод IP-адреса сервера
    string server_ip = get_local_ip();
    cout << "Server started at " << server_ip << ", port " << port << endl;

    while (true) {
        // Прослушивание порта
        if (listen(servSock, 10) == SOCKET_ERROR) {
            cout << "Unable to listen" << endl;
            WSACleanup();
            return SOCKET_ERROR;
        }

        cout << "Waiting for connections..." << endl;
        SOCKADDR_IN clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSock = accept(servSock, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSock == INVALID_SOCKET) {
            cout << "Unable to accept connection" << endl;
            continue;
        }

        cout << "New connection accepted from " << inet_ntoa(clientAddr.sin_addr) << ", port " << ntohs(clientAddr.sin_port) << endl;

        // Работа с клиентом
        char buffer[256] = { 0 };
        int retVal = recv(clientSock, buffer, sizeof(buffer), 0);
        if (retVal > 0) {
            cout << "Received: " << buffer << endl;
            string response = "You sent: " + string(buffer) + "\n";
            send(clientSock, response.c_str(), response.length(), 0);
        }
        closesocket(clientSock);
    }

    closesocket(servSock);
    WSACleanup();
    return 0;
}
