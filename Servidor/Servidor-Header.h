#pragma once
#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <strsafe.h>

#define _CRT_SECURE_NO_WARNINGS

//----------------------------------- [CONSTANTS] ------------------------------------
#define MaxMonsters 10
#define MaxClients 10
#define MaxRooms 10

#define BUFSIZE 2048

#define QUEMSZ 60
#define MSGTXTSZ 60
#define Msg_Sz sizeof(Msg)

#define N_MAX_LEITORES 10
#define TAM 256


//---------------------------------- [HANDLES and VARIABLES] ------------------------------
HANDLE PipeLeitores[N_MAX_LEITORES];
int total;
BOOL fim = FALSE;

HANDLE hPipe, hPipe2;
#define PIPE_NAME TEXT("\\\\.\\pipe\\teste")

//DWORD WINAPI RecebeLeitores(LPVOID param);  //??

HANDLE hThread;


HANDLE WriteReady;



// FUNCTIONS DECLARATION



//---------------------------CLIENT OPERATORS---------------------
void InitializeClients();
void ResetClient(int i);
void addClient(HANDLE HClient);
void RemoveClient(HANDLE HClient);

int writeClienteASINC(HANDLE hPipe, Msg msg);
int BroadcastClients(Msg msg);