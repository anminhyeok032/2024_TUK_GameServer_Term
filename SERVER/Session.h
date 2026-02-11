#pragma once
#include "OVER.h"

// [Session]
// 플레이어와 NPC의 공통 부모 클래스
// 네트워크 연결(OVER), 위치(x,y), 스탯(hp, level) 등의 공통 속성을 관리

class SESSION
{
public:
	// player와 npc의 공통 필요 속성
	OVER recv_over_;
	int id_;
	short x_, y_;
	int max_hp_, hp_;
	int visual_;
	int	level_;
	char name_[NAME_SIZE];

	// 현재 위치한 섹터
	std::pair<int, int> current_sector_;
	std::set<std::pair<int, int>> around_sector_;
	std::unordered_set<int> view_list_;

	// packet 조립용 버퍼
	std::vector<char> prev_packet_;

	// mutex
	std::mutex mut_view_;	// view_list_에 대한 mutex
	std::mutex mut_state_;	// state_에 대한 mutex

	// 현재 객체 상태 정보
	OBJECT_STATE state_;

	// 시간 정보
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
	virtual ~SESSION() {} // 가상 소멸자 (중요)

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
	
	// 공격 패킷 인자 추가 (시각화용)
	virtual void SendAttackPacket(int attacker_id, int damaged_id, int exp, char attack_type, char direction) {}
	
	virtual void ProcessPacket(char* packet) {};

	virtual void DBLogin(SQLHDBC& hdbc) {};
	virtual void DBLogout(SQLHDBC& hdbc) {};
	virtual void SaveToRedis() {};


	virtual SOCKET GetSocket() { return INVALID_SOCKET; }
	virtual void SetSocket(SOCKET socket) {};
	virtual void CloseSocket() {};

	// Npc
	virtual void SetActive(bool active) {}
	virtual bool GetActive() { return false; }
	virtual bool IsPlayerExist() { return false; };
	virtual void DoMove(int target_id) {};
	virtual void WakeUpNpc(int p_id) {};
	virtual void SetStartPos(int x, int y) {}

	// 섹터
	void PutInSector();
};

extern std::array<std::unique_ptr<SESSION>, MAX_NPC + MAX_USER> objects;
