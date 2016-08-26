#pragma once
#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#include "Servidor-Header.h"

typedef struct {		//Estrutura que define as caracteristicas de cada um dos jogadores, cada cliente que se conecta deve inicializar esta estrutura.

	DWORD XX;	//Player 'x' position.
	DWORD YY;	//Player 'y' position.
	DWORD Health;
	DWORD Slowness;
	HANDLE id;		//Handle of the process/thread
	//	DWORD enviaatodos;
	DWORD Identification;
	//DWORD bi;  //				{WHAT IS IT?}

} Client;

typedef struct {
	DWORD XX;
	DWORD YY;
	DWORD Health;
	DWORD MonsterType;
} Monster;  // Struct that defines the monsters

typedef struct {
	//Caracteristicas das salas
	DWORD position;
} Room;	  // Struct that defines each room of the game map.

typedef struct {

	Client Clients[MaxClients];
	Monster Monsters[MaxMonsters];
	Room Map[MaxRooms];

}FatherStructure;

FatherStructure Dungeon;

typedef struct {
	TCHAR quem[QUEMSZ];
	TCHAR msg[MSGTXTSZ];
}Msg;