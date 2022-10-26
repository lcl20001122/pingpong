#pragma once
#include"winsock2.h"
#include"iostream"
#include<queue>
#pragma comment(lib,"ws2_32")
#define BUF_SIZE 100

struct ServerSocket
{
    SOCKET sock;
    SOCKADDR_IN servAdr;
};

void init();

static DWORD WINAPI _ConnectionThread(LPVOID lpParam);

static DWORD WINAPI _WorkerThread(LPVOID lpParam);