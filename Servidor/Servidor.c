//Trabalho epoca especial
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <io.h>
#include <fcntl.h>
#include "Estrutura.h"


void readTChars(TCHAR * p, int maxchars)
{
	int len;
	_fgetts(p, maxchars, stdin);
	len = _tcslen(p);
	if (p[len - 1] == TEXT('\n'))
	{
		p[len - 1] = TEXT('\0');
	}
}

void pressEnter()
{
	TCHAR somekeys[25];
	_tprintf(TEXT("\nPress Enter > "));
	readTChars(somekeys, 25);
}

DWORD WINAPI InstanceThread(LPVOID lpvParam);
//
//#define MAXCLIENTES 10
//HANDLE clientes[MAXCLIENTES];

//----------------------------------------------- [CLIENT OPERATIONS] ---------------------------------------------------------

//---------------------------------------- INITIALIZE CLIENTS ---------------------------------------------------------------
void InitializeClients()
{
	int i = 0;
	for (i = 0; i < MaxClients; i++)
	{
		ResetClient(i);		//Poderá nao funcionar devido ao NULL
		Dungeon.Clients[i].id = NULL;				//	Handle of the client
		Dungeon.Clients[i].Identification = NULL;  //	Number of the client which is atribuited by login order / array order
	}
}

//---------------------------------------------- ADD CLIENTS ---------------------------------------------------------------

void addClient(HANDLE HClient)
{								
	int i;
	for (i = 0; i < MaxClients; i++)
	{
		if (Dungeon.Clients[i].id == NULL)
		{
			//ResetClient(HClient, i);				//Aqui não é feito reset aos clients para haver a possibilidade de adicionar um cliente com certas caracteristicas, ex: um save do game
			Dungeon.Clients[i].id = HClient;		//	Changes the client Handle in the structure from Null to the one given.
			Dungeon.Clients[i].Identification = i;	//	Atributes the client identification that is his array number.

			return;
		}
	}
}

//-------------------------------------------- REMOVE CLIENTS ---------------------------------------------------------------
void RemoveClient(HANDLE HClient)
{
	for (int i = 0; i < MaxClients; i++)
	{
		if (Dungeon.Clients[i].id == HClient)
		{
			ResetClient(i);
			Dungeon.Clients[i].id = NULL;
			return;
		}
	}
}

void ResetClient(int i)
{
	Dungeon.Clients[i].Slowness = 5;
	Dungeon.Clients[i].Health = 10;
	return;
}


int writeClienteASINC(HANDLE hPipe, Msg msg)
{
	DWORD cbWritten = 0;
	BOOL fSuccess = FALSE;

	OVERLAPPED OverlWr = { 0 };
	ZeroMemory(&OverlWr, sizeof(OverlWr));
	ResetEvent(WriteReady);
	OverlWr.hEvent = WriteReady;

	fSuccess = WriteFile(hPipe, &msg, Msg_Sz, &cbWritten, &OverlWr);

	WaitForSingleObject(WriteReady, INFINITE);

	GetOverlappedResult(hPipe, &OverlWr, &cbWritten, FALSE);
	if (cbWritten < Msg_Sz)
	{
		_tprintf(TEXT("\nWriteFile não escreveu toda a informação. Erro = %d"), GetLastError());
	}

	return 1;
}

int BroadcastClients(Msg msg)
{
	int i, numwrites = 0;
	for (i = 0; i < MaxClients; i++)
	{
		if (Dungeon.Clients[i].id != 0)
		{
			numwrites += writeClienteASINC(Dungeon.Clients[i].id, msg);
		}
	}
	return numwrites;
}


int _tmain(VOID)
{
	BOOL fConnected = FALSE;
	DWORD dwThreadId = 0;
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL;
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\pipeexemplo");

	_setmode(_fileno(stdout), _O_WTEXT);

	WriteReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (WriteReady == NULL)
	{
		_tprintf(TEXT("\nServidor: não foi possível criar o evento Write. Mais vale parar já!"));
		return 1;
	}

	InitializeClients();

	while (1)
	{
		hPipe = CreateNamedPipe
			(lpszPipename,
				PIPE_ACCESS_DUPLEX |
				FILE_FLAG_OVERLAPPED,
				PIPE_TYPE_MESSAGE |
				PIPE_READMODE_MESSAGE |
				PIPE_WAIT,
				PIPE_UNLIMITED_INSTANCES,
				BUFSIZE,
				BUFSIZE,
				5000,
				NULL);

		if (hPipe == INVALID_HANDLE_VALUE)
		{
			_tprintf(TEXT("\nCreateNamedPipe falhou, erro = %d"), GetLastError());
			return -1;
		}

		_tprintf(TEXT("\nServidor a aguardar que um cliente se ligue"));

		fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (fConnected)
		{
			hThread = CreateThread(
				NULL,
				0,
				InstanceThread,
				(LPVOID)hPipe,
				0,
				&dwThreadId);

			if (hThread == NULL)
			{
				_tprintf(TEXT("\nErro na criação da Thread. Erro = %d"), GetLastError());
				return -1;
			}
			else
			{
				CloseHandle(hThread);
			}
		}
		else
		{
			CloseHandle(hPipe);
		}
	}
	return 0;
}

DWORD WINAPI InstanceThread(LPVOID lpvParam)
{
	Msg Pedido, Resposta;
	DWORD cbBytesRead = 0, cbReplyBytes = 0;
	int numresp = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = (HANDLE)lpvParam;

	HANDLE ReadReady;
	OVERLAPPED OverlRd = { 0 };

	_tcscpy(Resposta.quem, TEXT("SRV"));

	if (hPipe == NULL)
	{
		_tprintf(TEXT("\nErro - o handle enviado no param da thread é nulo"));
		return -1;
	}

	ReadReady = CreateEvent(
		NULL,
		TRUE,
		FALSE,
		NULL);

	if (ReadReady == NULL)
	{
		_tprintf(TEXT("\nServidor: não foi possivel criar o evento Read. Mais vale parar já"));
		return 1;
	}

	addClient(hPipe);

	while (1)
	{
		ZeroMemory(&OverlRd, sizeof(OverlRd));
		ResetEvent(ReadReady);
		OverlRd.hEvent = ReadReady;
		fSuccess = ReadFile(
			hPipe,
			&Pedido,
			Msg_Sz,
			&cbBytesRead,
			&OverlRd);

		WaitForSingleObject(ReadReady, INFINITE);

		GetOverlappedResult(hPipe, &OverlRd, &cbBytesRead, FALSE);
		if (cbBytesRead < Msg_Sz)
		{
			_tprintf(TEXT("\nReadFile não leu os dados todos. Erro = %d"), GetLastError());
		}

		_tprintf(TEXT("\nServidor: Recebi (?) de: [%s] msg: [%s]"), Pedido.quem, Pedido.msg);
		_tcscpy(Resposta.msg, Pedido.quem);
		_tcscat(Resposta.msg, TEXT(": "));
		_tcscat(Resposta.msg, Pedido.msg);

		numresp = BroadcastClients(Resposta);
		_tprintf(TEXT("\nServidor: %d respostas enviadas"), numresp);
	}
	RemoveClient(hPipe);

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);

	_tprintf(TEXT("\nThread dedicada ao Cliente a terminar"));
	return 1;
}
