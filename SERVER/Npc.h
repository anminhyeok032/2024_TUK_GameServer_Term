#pragma once
#include "Session.h"

class Npc : public SESSION
{
public:
	int start_x_, start_y_;
	lua_State* L_;
	std::mutex mut_lua_;
	std::atomic_bool active_;

public:
	Npc() : active_(false) {}
	~Npc() {}

	void SetActive(bool active) { active_ = active; }
	bool GetActive() { return active_; }

	void SetLua(lua_State* L) { L_ = L; }

	void SendAddObjectPacket(int c_id);
	void SendRemoveObjectPacket(int c_id);
	void SendAttackPacket(int attacker_id, int damaged_id, int exp);

	// npc 주변에 플레이어가 있는지 검사하는 함수
	bool IsPlayerExist();
	void WakeUpNpc(int p_id);
	void SetAiLua();
	void DoRandomMove(int target_id);

	// 원래 위치 지정
	void SetStartPos(int x, int y) { start_x_ = x; start_y_ = y; }
};