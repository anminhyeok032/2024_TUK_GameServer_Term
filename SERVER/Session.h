#pragma once
#include "OVER.h"



class SESSION
{
public:
	// player와 npc의 공통 멤버 변수
	OVER recv_over_;
	int id_;
	short x_, y_;
	int max_hp_, hp_;
	int visual_;
	int	level_;
	char name_[NAME_SIZE];

	// 섹터 정보와 뷰리스트
	std::pair<int, int> current_sector_;
	std::set<std::pair<int, int>> around_sector_;
	std::unordered_set<int> view_list_;

	// packet 재조립
	std::vector<char> prev_packet_;

	// mutex
	std::mutex mut_view_;	// view_list_에 대한 mutex
	std::mutex mut_state_;	// state_에 대한 mutex

	// 현재 객체 사용 여부
	OBJECT_STATE state_;

	// 시간 계산용
	int last_move_time_;

public:
	SESSION() 
	{
		id_ = -1;
		x_ = y_ = 0;
		hp_ = max_hp_ = visual_ = 0;
		name_[0] = 0;
		state_ = OS_FREE;
		prev_packet_.clear();
		current_sector_ = { -99, -99 };
	}
	~SESSION() {}

	// Player
	virtual void DoReceive() {};
	virtual void SendLoginInfoPacket() {};
	virtual void SendLoginFailPacket() {};
	virtual void DoSend(void* packet) {};
	virtual void SendMovePacket(int c_id) {};
	virtual void SendAddObjectPacket(int c_id) {};
	virtual void SendRemoveObjectPacket(int c_id) {};
	virtual void SendChatPacket(int c_id, char mess[CHAT_SIZE]) {}
	virtual void SendStatChangePacket() {};
	virtual void SendAttackPacket(int attacker_id, int damaged_id, int exp) {}
	virtual void ProcessPacket(char* packet) {};

	virtual void DBLogin(SQLHDBC& hdbc) {};
	virtual void DBLogout(SQLHDBC& hdbc) {};


	virtual SOCKET GetSocket() { return INVALID_SOCKET; }
	virtual void SetSocket(SOCKET socket) {};
	virtual void CloseSocket() {};

	// Npc
	virtual void SetActive(bool active) {}
	virtual bool GetActive() { return false; }
	virtual bool IsPlayerExist() { return false; };
	virtual void DoRandomMove() {};

	// 공통
	void PutInSector();
};

extern std::array<std::unique_ptr<SESSION>, MAX_NPC + MAX_USER> objects;
