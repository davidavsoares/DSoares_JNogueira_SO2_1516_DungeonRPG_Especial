#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <io.h>
#include <fcntl.h>

#define BUFSIZE 2048

#define QUEMSZ 60
#define MSGTXTSZ 60

#define PIPE_SERVER_CLIENT TEXT("\\\\.\\pipe\\pipe1")
#define PIPE_CLIENT_SERVER TEXT("\\\\.\\pipe\\pipe2")

typedef struct {
	TCHAR quem[QUEMSZ];
	TCHAR msg[MSGTXTSZ];
}Msg;

#define Msg_Sz sizeof(Msg)

int get_code()
{
	int ch = getch();
	if (ch == 0 || ch == 224)
		ch = 256 + getch();
	return ch;
}

enum
{
	KEY_ESC = 27,
	ARROW_UP = 256 + 72,
	ARROW_DOWN = 256 + 80,
	ARROW_LEFT = 256 + 75,
	ARROW_RIGHT = 256 + 77
};

int recebe_comando()
{
	int ch;
	ch = get_code();

	switch (ch)
	{
	case ARROW_UP:
		return 72;
		break;
	case ARROW_DOWN:
		return 88;
		break;
	case ARROW_LEFT:
		return 75;
		break;
	case ARROW_RIGHT:
		return 77;
		break;
	case KEY_ESC:
		return 27;
		break;
	}
	return 10;

}

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

void readTArrows(TCHAR * p, int maxchars)
{
	TCHAR up[3] = TEXT("UP");
	TCHAR down[5] = TEXT("DOWN");
	TCHAR left[5] = TEXT("LEFT");
	TCHAR right[6] = TEXT("RIGHT");
	TCHAR esc[4] = TEXT("ESC");
	TCHAR cont[5] = TEXT("CONT");
	
	if (recebe_comando() == 72)
	{
		_tcscpy_s(p, _tcslen(up)*sizeof(TCHAR), up);
		
	}
	else
	{
		if (recebe_comando() == 88)
		{
			_tcscpy_s(p, _tcslen(down)*sizeof(TCHAR), down);
			
		}
		else
		{
			if (recebe_comando() == 75)
			{
				_tcscpy_s(p, _tcslen(left)*sizeof(TCHAR), left);
				
			}
			else
			{
				if (recebe_comando() == 77)
				{
					_tcscpy_s(p, _tcslen(right)*sizeof(TCHAR), right);
					
				}
				else
				{
					if (recebe_comando() == 27)
					{
						_tcscpy_s(p, _tcslen(esc)*sizeof(TCHAR), esc);
						
					}
					else
					{
						_tcscpy_s(p, _tcslen(cont)*sizeof(TCHAR), cont);
						
					}
				}
			}
		}
	}


}

void pressEnter()
{
	TCHAR somekeys[25];
	_tprintf(TEXT("\nPress Enter > "));
	readTChars(somekeys, 25);
}

DWORD WINAPI ThreadClienteReader(LPVOID lpvParam);

int DeveContinuar = 1;
int ReaderAlive = 0;

int _tmain(int argc, TCHAR *argv[])
{
	HANDLE hPipeSC, hPipeCS;
	BOOL fSuccess = FALSE;
	DWORD cbWritten, dwMode;
	//LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\pipeexemplo");

	Msg MsgToSend;
	HANDLE hThread;
	DWORD dwThreadId = 0;

	_setmode(_fileno(stdout), _O_WTEXT);

	_tprintf(TEXT("Escreve nome > "));
	readTChars(MsgToSend.quem, QUEMSZ);

	while (1)
	{
		hPipeSC = CreateFile(
			PIPE_SERVER_CLIENT,
			GENERIC_READ,
			0 /*| FILE_SHARE_READ | FILE_SHARE_WRITE*/,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL/*0 | FILE_FLAG_OVERLAPPED*/,
			NULL);

		if (hPipeSC != INVALID_HANDLE_VALUE)
		{
			break;
		}

		hPipeCS = CreateFile(
			PIPE_CLIENT_SERVER,
			GENERIC_WRITE,
			0/* | FILE_SHARE_READ | FILE_SHARE_WRITE*/,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL/*0 | FILE_FLAG_OVERLAPPED*/,
			NULL);

		if (hPipeCS != INVALID_HANDLE_VALUE)
		{
			break;
		}

		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			_tprintf(TEXT("\nCreate file deu erro e não foi BUSY. Erro = %d\n"), GetLastError());
			pressEnter();
			return -1;
		}

		if (!WaitNamedPipe(PIPE_SERVER_CLIENT, 30000))
		{
			_tprintf(TEXT("\nEsperei por uma instância durante 30 segundos. Desisto. Sair"));
			pressEnter();
			return -1;
		}
	}
	//_-----------------------------------QUANDO ISTO NAO STAVA COMENTADO NAO FUNCIONAVA; DAVA: SetNamedPipeHandleState falhou. Erro = 5
	//dwMode = PIPE_READMODE_MESSAGE;
	//fSuccess = SetNamedPipeHandleState(
	//	hPipeSC,
	//	&dwMode,
	//	NULL,
	//	NULL);

	//if (!fSuccess)
	//{
	//	_tprintf(TEXT("\nSetNamedPipeHandleState falhou. Erro = %d\n"), GetLastError());
	//	pressEnter();
	//	return -1;
	//}

	hThread = CreateThread(
		NULL,
		0,
		ThreadClienteReader,
		(LPVOID)hPipeSC,
		0,
		&dwThreadId);

	if (hThread == NULL)
	{
		_tprintf(TEXT("\nErro na criação da thread. Erro = %d"), GetLastError());
		return -1;
	}

	HANDLE WriteReady;
	OVERLAPPED OverlWr = { 0 };

	WriteReady = CreateEvent(
		NULL,
		TRUE,
		FALSE,
		NULL);

	if (WriteReady == NULL)
	{
		_tprintf(TEXT("\nCliente: não foi possível criar o Evento. Mais vale parar já"));
		return 1;
	}

	_tprintf(TEXT("\nLigação estabelecida. \"exit\" para sair"));

	while (1)
	{
		//Faz a leitura das setas
		_tprintf(TEXT("\n%s > "), MsgToSend.quem);
		
		readTArrows(MsgToSend.msg, MSGTXTSZ);
		if (_tcscmp(TEXT("exit"), MsgToSend.msg) == 0)
		{
			break;
		}
		_tprintf(TEXT("\nA enviar %d bytes: \"%s\""), Msg_Sz, MsgToSend.msg);

		ZeroMemory(&OverlWr, sizeof(OverlWr));
		ResetEvent(WriteReady);
		OverlWr.hEvent = WriteReady;

		fSuccess = WriteFile(
			hPipeCS/*hPipeSC*/,
			&MsgToSend,
			Msg_Sz,
			&cbWritten,
			&OverlWr);

		WaitForSingleObject(WriteReady, INFINITE);
		_tprintf(TEXT("\nWrite concluido"));

		GetOverlappedResult(hPipeCS/*hPipeSC*/, &OverlWr, &cbWritten, FALSE);
		if (cbWritten < Msg_Sz)
		{
			_tprintf(TEXT("\nWriteFile TALVEZ falhou. Erro = %d"), GetLastError());
		}
		_tprintf(TEXT("\nMensagem enviada"));
	}
	_tprintf(TEXT("\nEncerrar a thread ouvinte"));

	DeveContinuar = 0;
	if (ReaderAlive)
	{
		WaitForSingleObject(hThread, 3000);
		_tprintf(TEXT("\nThread reader encerrada ou timeout"));
	}
	_tprintf(TEXT("\nCliente vai terminar ligação e sair"));

	CloseHandle(WriteReady);
	CloseHandle(hPipeSC);
	pressEnter();
	return 0;
}

DWORD WINAPI ThreadClienteReader(LPVOID lpvParam)
{
	Msg FromServer;

	DWORD cbBytesRead = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = (HANDLE)lpvParam;

	HANDLE ReadReady;
	OVERLAPPED OverlRd = { 0 };

	if (hPipe == NULL)
	{
		_tprintf(TEXT("\nThread Reader - o handle recebido no param da thread é nulo\n"));
		return -1;
	}

	ReadReady = CreateEvent(
		NULL,
		TRUE,
		FALSE,
		NULL);

	if (ReadReady == NULL)
	{
		_tprintf(TEXT("\nCliente: não foi possível criar o Evento Read. Mais vale parar já"));
		return 1;
	}

	ReaderAlive = 1;
	_tprintf(TEXT("\nThread Reader - a receber mensagens\n"));

	while (DeveContinuar)
	{
		ZeroMemory(&OverlRd, sizeof(OverlRd));
		OverlRd.hEvent = ReadReady;
		ResetEvent(ReadReady);

		fSuccess = ReadFile(
			hPipe,
			&FromServer,
			Msg_Sz,
			&cbBytesRead,
			&OverlRd);

		WaitForSingleObject(ReadReady, INFINITE);
		_tprintf(TEXT("\nRead concluido"));

		GetOverlappedResult(hPipe, &OverlRd, &cbBytesRead, FALSE);

		if (cbBytesRead < Msg_Sz)
		{
			_tprintf(TEXT("\nReadFile falhou. Erro = %d"), GetLastError());
		}

		_tprintf(TEXT("\nServidor disse: [%s]"), FromServer.msg);
	}

	ReaderAlive = 0;
	_tprintf(TEXT("\nThread Reader a terminar. \n"));
	return 1;
}