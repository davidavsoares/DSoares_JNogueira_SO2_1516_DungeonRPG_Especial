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


int writeClienteASINC(HANDLE hPipe, Msg msg)		//Envia msg a um cliente!
{
	DWORD cbWritten = 0;
	BOOL fSuccess = FALSE;

	OVERLAPPED OverlWr = { 0 };		//Overlapped serve para controlar eventos
	ZeroMemory(&OverlWr, sizeof(OverlWr));
	ResetEvent(WriteReady);
	OverlWr.hEvent = WriteReady;

	fSuccess = WriteFile(hPipe, &msg, Msg_Sz, &cbWritten, &OverlWr);		//Escreve no pipe (Onde escrever, Endereço do buffer que contem os dados, tamanho, variavel que recebe o numero de bits escritos, 

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

DWORD WINAPI ConnectClients(LPVOID param)
{
	BOOL fConnected = FALSE;
	DWORD dwThreadId = 0;

	while (1)
	{
		hPipeSC = CreateNamedPipe
		(PIPE_SERVER_CLIENT,
			PIPE_ACCESS_OUTBOUND/* |
								FILE_FLAG_OVERLAPPED*/,
			PIPE_TYPE_MESSAGE |
			PIPE_READMODE_MESSAGE |
			PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES, //OR MaxClients
			BUFSIZE,
			BUFSIZE,
			5000,
			NULL);

		if (hPipeSC == INVALID_HANDLE_VALUE)
		{
			_tprintf(TEXT("\nCreateNamedPipe failed, error = %d"), GetLastError());
			return -1;
		}

		hPipeCS = CreateNamedPipe
		(PIPE_CLIENT_SERVER,
			PIPE_ACCESS_INBOUND/* |
							   FILE_FLAG_OVERLAPPED*/,
			PIPE_TYPE_MESSAGE |
			PIPE_READMODE_MESSAGE |
			PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,//OR MaxClients
			BUFSIZE,
			BUFSIZE,
			5000, //TIMEOUT
			NULL);

		if (hPipeCS == INVALID_HANDLE_VALUE)
		{
			_tprintf(TEXT("\nCreateNamedPipe failed, error = %d"), GetLastError());
			return -1;
		}

		_tprintf(TEXT("\nServidor a aguardar que um cliente se ligue"));

		fConnected = ConnectNamedPipe(hPipeSC, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);	//SE OS DOIS PIPES ESTIVEREM CONECTADOS INICIA A THREAD PARA OS CLIENTES FAZEREM AS SUAS COISAS

		if (fConnected)
		{
			if (ConnectNamedPipe(hPipeCS, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED))
			{
				hThread = CreateThread(
					NULL,
					0,
					InstanceThread,
					(LPVOID)hPipeSC,
					0,
					&dwThreadId);

				if (hThread == NULL)
				{
					_tprintf(TEXT("\nError creating Thread. Error = %d"), GetLastError());
					return -1;
				}
				else
				{
					CloseHandle(hThread);
				}
			}
			else
			{
				CloseHandle(hPipeSC);
			}
		}
	}

}

DWORD WINAPI InstanceThread(LPVOID lpvParam)		//Retirado do PDF de overlapped,  thread que vai atender os clientes = TP PONTEs
{
	Msg Pedido, Resposta;
	DWORD cbBytesRead = 0, cbReplyBytes = 0;
	int numresp = 0; //fora
	BOOL fSuccess = FALSE;
	HANDLE hPipe = (HANDLE)lpvParam;

	HANDLE ReadReady;
	OVERLAPPED OverlRd = { 0 };

	_tcscpy(Resposta.quem, TEXT("SRV"));

	if (hPipe == NULL)
	{
		_tprintf(TEXT("\nError - The handle sent to the thread's param is NULL"));
		return -1;
	}

	ReadReady = CreateEvent(
		NULL,
		TRUE,
		FALSE,
		NULL); //(Atributos Seguranca, Manual Reset, initial state of the event object is signaled, The name of the event object If lpName is NULL, the event object is created without a name.)

	if (ReadReady == NULL)
	{
		_tprintf(TEXT("\nServidor: não foi possivel criar o evento Read. Mais vale parar já"));
		return 1;
	}

	addClient(hPipe);//é atribuido o handle do pipe ao cliente na estrutura (Handle Servidor -> Cliente)

	while (1)// Esta thread vai estar constantemente a fazer as operacoes do cliente
	{
		ZeroMemory(&OverlRd, sizeof(OverlRd));
		ResetEvent(ReadReady);
		OverlRd.hEvent = ReadReady;

		fSuccess = ReadFile(
			hPipe,//c->s?
			&Pedido,
			Msg_Sz,
			&cbBytesRead,
			&OverlRd);

		WaitForSingleObject(ReadReady, INFINITE);

		GetOverlappedResult(hPipe, &OverlRd, &cbBytesRead, FALSE);

		if (cbBytesRead < Msg_Sz)
		{
			_tprintf(TEXT("\nReadFile didn't read all the data. Erro = %d"), GetLastError());
		}

		// APAGADO
		_tprintf(TEXT("\nServidor: Recebi (?) de: [%s] msg: [%s]"), Pedido.quem, Pedido.msg);
		_tcscpy(Resposta.msg, Pedido.quem);
		_tcscat(Resposta.msg, TEXT(": "));
		_tcscat(Resposta.msg, Pedido.msg);

		numresp = BroadcastClients(Resposta);		//Faz broadcast da msg
		_tprintf(TEXT("\nServidor: %d respostas enviadas"), numresp);
	}
	RemoveClient(hPipe);

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
	//Apagado

	_tprintf(TEXT("\nThread dedicada ao Cliente a terminar"));
	return 1;

	//	if (!fSuccess || cbBytesRead < sizeof(Msg))
	//	{
	//		_tcscpy(answer.msg, TEXT(""));
	//
	//		// Trata do comando
	//		processCommand(param, request, answer);
	//	}
	//};
	//
	//shutdownClient(hPipe);
}

int _tmain(VOID)
{

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
	
	//Com esta thread pretende-se atender os clientes
	hThread = CreateThread(
		NULL,
		0,
		ConnectClients,
		NULL,
		0,
		&dwThreadId);

	WaitForSingleObject(hThread, INFINITE);

	if (hThread == NULL)
	{
		_tprintf(TEXT("\nErro na criação da Thread. Erro = %d"), GetLastError());
		return -1;
	}
	else
	{
		CloseHandle(hThread);
	}

	/*while (1)
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
	}*/
	return 0;
}




