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

	// npc �ֺ��� �÷��̾ �ִ��� �˻��ϴ� �Լ�
	bool IsPlayerExist();
	void WakeUpNpc(int p_id);
	void SetAiLua();
	void DoRandomMove(int target_id);

	// ���� ��ġ ����
	void SetStartPos(int x, int y) { start_x_ = x; start_y_ = y; }
};