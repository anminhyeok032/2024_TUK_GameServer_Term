#pragma once
#include "OVER.h"


class SESSION
{
public:
	// player와 npc의 공통 멤버 변수
	OVER recv_over_;
	int id_;
	short x_, y_;
	int visual_;
	char name_[NAME_SIZE];

	// 섹터 정보와 뷰리스트
	std::pair<int, int> current_sector;
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
		name_[0] = 0;
		state_ = OS_FREE;
		prev_packet_.clear();
		current_sector = { -99, -99 };
	}
	~SESSION() {}

	virtual void DoReceive() = 0;
	virtual void SendLoginInfoPacket() = 0;
	virtual void DoSend(void* packet) = 0;
	virtual void SendMovePacket(int c_id) = 0;
	virtual void SendAddObjectPacket(int c_id) = 0;
	virtual void SendRemoveObjectPacket(int c_id) = 0;
	virtual void ProcessPacket(char* packet) = 0;

	virtual SOCKET GetSocket() = 0;
	virtual void SetSocket(SOCKET socket) = 0;

	virtual void SetActive(bool active) {}
	virtual bool GetActive() { return false; }

	void PutInSector();
};

extern std::array<std::unique_ptr<SESSION>, MAX_NPC + MAX_USER> objects;