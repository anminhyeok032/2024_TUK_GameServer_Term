#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>

#include <unordered_set>
#include <array>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <concurrent_priority_queue.h>

#include <thread>
#include <mutex>
#include <chrono>

#include "include/lua.hpp"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "lua54.lib")

//// global 변수
constexpr int BUF_SIZE = 200;

extern SOCKET g_server_socket, g_client_socket;
extern HANDLE g_h_iocp;

struct Sector
{
	std::unordered_set<int> sec_id_;
	std::mutex mut_sector_;
};
extern std::map <std::pair<int, int>, Sector> g_ObjectSector;
extern std::vector<int> g_player_list;

constexpr int VIEW_RANGE = 5;
constexpr int SEC_RANGE = VIEW_RANGE;
constexpr int SEC_ROW = 10;
constexpr int SEC_COL = 10;

void print_error(const char* msg, int err_no);	
bool CanSee(int curr, int other);
bool IsNpc(int a);
bool IsPlayer(int a);
void disconnect(int c_id);

enum COMP_KEY
{
	KEY_ACCEPT = 0,
	KEY_SEND,
	KEY_RECV,
	KEY_NPC_RANDOM_MOVE,
	KEY_NPC_MOVE_TO_PLAYER,
	KEY_NPC_ATTACK,
};

enum OBJECT_STATE
{
	OS_FREE = 0,
	OS_ACTIVE,
	OS_INGAME,
	OS_DEAD
};

enum OBJECT_TYPE
{
	OT_PLAYER = 0,
	OT_NPC
};

enum EVENT_TYPE
{
	EV_NPC_RANDOM_MOVE = 0,
	EV_HEAL,
	EV_ATTACK,
	EV_SKILL
};

struct EVENT
{
	int id_;
	std::chrono::system_clock::time_point wakeup_time_;
	EVENT_TYPE e_type_;
	int target_id_;

	// priority_queue 정렬을 위한 < 오버라이드
	constexpr bool operator<(const EVENT& other) const
	{
		return this->wakeup_time_ > other.wakeup_time_;
	}
};

// lock-free thread-safe 우선순위 큐
// clear는 thread-safe하지 않음
extern concurrency::concurrent_priority_queue<EVENT> g_event_queue;


