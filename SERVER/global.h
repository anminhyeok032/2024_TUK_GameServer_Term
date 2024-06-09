#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <unordered_set>
#include <thread>
#include <vector>
#include <mutex>
#include <array>
#include <map>
#include <set>
#include "include/lua.hpp"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "lua54.lib")

//// global º¯¼ö
constexpr int BUF_SIZE = 200;

extern SOCKET g_server_socket, g_client_socket;
extern HANDLE g_h_iocp;

struct Sector
{
	std::unordered_set<int> sec_id_;
	std::mutex mut_sector_;
};
extern std::map <std::pair<int, int>, Sector> g_ObjectSector;

constexpr int VIEW_RANGE = 5;
constexpr int SEC_RANGE = VIEW_RANGE;
constexpr int SEC_ROW = 10;
constexpr int SEC_COL = 10;

void print_error(const char* msg, int err_no);	
bool CanSee(int curr, int other);
bool IsNpc(int a);
void disconnect(int c_id);

enum COMP_KEY
{
	ACCEPT = 0,
	SEND,
	RECV
};

enum OBJECT_STATE
{
	OS_FREE = 0,
	OS_ACTIVE,
	OS_INGAME
};

enum OBJECT_TYPE
{
	OT_PLAYER = 0,
	OT_NPC
};
