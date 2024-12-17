#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include <winsock2.h>
#include <string>
#include <iostream>
using namespace std;

int main() {
    WORD ver = MAKEWORD(2, 2);
    WSADATA wsaData;
    int retVal = 0;
    WSAStartup(ver, (LPWSADATA)&wsaData);

    // Создаем сокет
    SOCKET clientSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSock == SOCKET_ERROR) {
        printf("Unable to create socket\n");
        WSACleanup();
        return 1;
    }

    string ip;
    int port;
    cout << "Enter server IP: ";
    cin >> ip;
    cout << "Enter server port: ";
    cin >> port;
    cin.ignore();  // Очистка буфера после ввода IP и порта

    SOCKADDR_IN serverInfo;
    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    serverInfo.sin_port = htons(port);

    // Присоединение к серверу
    retVal = connect(clientSock, (LPSOCKADDR)&serverInfo, sizeof(serverInfo));
    if (retVal == SOCKET_ERROR) {
        printf("Unable to connect\n");
        WSACleanup();
        return 1;
    }

    printf("Connection made successfully\n");

    string input;
    while (true) {
        // Ввод строки текста от пользователя
        printf("Enter a sentence (or 'exit' to quit): ");
        getline(cin, input);

        if (input == "exit") {
            // Отправляем на сервер команду выхода, но не закрываем сразу клиент
            retVal = send(clientSock, input.c_str(), input.length(), 0);
            if (retVal == SOCKET_ERROR) {
                printf("Unable to send\n");
                WSACleanup();
                return 1;
            }

            printf("Closing client connection...\n");
            break;  // Выходим из цикла, завершаем клиент
        }

        // Проверка на слишком длинную строку
        if (input.length() > 255) {
            cout << "Input is too long. Please enter a shorter sentence." << endl;
            continue;
        }

        // Отправка строки на сервер
        retVal = send(clientSock, input.c_str(), input.length(), 0);
        if (retVal == SOCKET_ERROR) {
            printf("Unable to send\n");
            WSACleanup();
            return 1;
        }

        // Ожидание ответа от сервера
        char szResponse[256] = { 0 };
        retVal = recv(clientSock, szResponse, sizeof(szResponse), 0);
        if (retVal == SOCKET_ERROR) {
            printf("Unable to receive\n");
            WSACleanup();
            return 1;
        }

        // Печать ответа от сервера
        printf("Server response: %s\n", szResponse);
    }

    closesocket(clientSock);
    WSACleanup();
    return 0;
}
