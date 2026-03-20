#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>

#include <unordered_set>
#include <array>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <string>
#include <cstring>
#include <concurrent_priority_queue.h>
#include <concurrent_queue.h>

#include <thread>
#include <mutex>
#include <chrono>
#include <cmath>

#include "include/lua.hpp"
#include "Constants.h"
#include "SnowflakeID.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "lua54.lib")

#include <windows.h>  
#include <sqlext.h>  
#define NAME_LEN 50  
#define PHONE_LEN 60

// redis 
#include <cpp_redis/cpp_redis>
#include <future>

//// global 
constexpr int BUF_SIZE = 4096;

extern SOCKET g_server_socket, g_client_socket;
extern HANDLE g_h_iocp;

struct Sector
{
	std::unordered_set<int> sec_id_;
	std::mutex mut_sector_;
};
extern std::map <std::pair<int, int>, Sector> g_ObjectSector;
extern std::unordered_set<int> g_player_list;
extern std::mutex g_mut_player_list;

void print_error(const char* msg, int err_no);	
bool CanSee(int curr, int other);
bool IsNpc(int a);
bool IsPlayer(int a);
bool IsMapItem(int a); // 맵 아이템인지 확인 (ID 범위로 판단)

// 빈 맵 아이템 슬롯 찾기 함수
int GetNewMapItemId();

void disconnect(int c_id);


extern std::unique_ptr<cpp_redis::client> g_redis_client;
extern SnowflakeIDGenerator g_snowflake;

// 
std::string WStringToString(const std::wstring& wstr);
std::string WStringToString(const SQLWCHAR* wstr);
bool ConnectWithRedis();

// DB struct queue
struct DBRequest
{
	enum DBType { LOGIN, LOGOUT, SAVE_REDIS, SAVE_ITEM, DELETE_ITEM };
	DBType db_type;
	int id;
	long long item_uid; // SAVE_ITEM, DELETE_ITEM
};
extern concurrency::concurrent_queue<DBRequest> g_db_request_queue;

// DB
void DisplayDBError(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
SQLHDBC ConnectWithDataBase();
SQLHSTMT AllocateStatement(SQLHDBC hdbc);
void DBWoker(SQLHDBC hdbc);
void ProcessDBRequest(const DBRequest& request, SQLHDBC& hdbc);


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

// 공격 타입 정의
// 데미지 공식: NORMAL = 레벨 * 3, AOE = 레벨 * 2
enum class AttackType : char
{
	NORMAL = 0,		// 단방향 평타 (레벨 * 3 데미지)
	AOE    = 1,		// 4방향 범위 공격 (레벨 * 2 데미지)
	// 추후 스킬 추가 시 여기에 확장
	// SKILL_FIRE = 2,
	// SKILL_ICE  = 3,
};

enum OBJECT_TYPE
{
	OT_PLAYER = 0,
	OT_NPC_AGRO = 1,
	OT_NPC_PEACE = 2,
	OT_MAP_ITEM = -1
};

enum EVENT_TYPE
{
	EV_NPC_RANDOM_MOVE = 0,
	EV_MOVE_TO_PLAYER,
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

	// priority_queue 
	constexpr bool operator<(const EVENT& other) const
	{
		return this->wakeup_time_ > other.wakeup_time_;
	}
};

// lock-free thread-safe 
// clear thread-safe
extern concurrency::concurrent_priority_queue<EVENT> g_event_queue;

void AddTimer(int id, EVENT_TYPE type, int ms, int target_id);
void DoAITimer();
int API_get_xy(lua_State* L);
int API_Move(lua_State* L);
int API_Attack(lua_State* L);
