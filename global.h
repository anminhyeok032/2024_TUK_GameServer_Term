#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <unordered_set>
#include <thread>
#include <vector>
#include <mutex>
#include <array>

//#include "protocol.h"

//#include "Session.h"
//#include "protocol.h"
//#include "Player.h"
//#include "Npc.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

//// global º¯¼ö
constexpr int BUF_SIZE = 200;

extern SOCKET g_server_socket, g_client_socket;
extern HANDLE g_h_iocp;

//std::array<SESSION*, MAX_NPC + MAX_USER> objects;

void print_error(const char* msg, int err_no);

enum COMP_KEY
{
	ACCEPT = 0,
	SEND,
	RECV
};

enum OBJECT_STATE
{
	ST_FREE = 0,
	ST_ACTIVE,
	ST_INGAME
};

