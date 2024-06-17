#pragma once
#include "Session.h"

class Npc : public SESSION
{
public:
	lua_State* L_;
	std::mutex mut_lua_;
	std::atomic_bool active_;

public:
	Npc() : active_(false) {}
	~Npc() {}

	void SetActive(bool active) { active_ = active; }
	bool GetActive() { return active_; }

	void SendAddObjectPacket(int c_id);
	void SendRemoveObjectPacket(int c_id);

	// npc �ֺ��� �÷��̾ �ִ��� �˻��ϴ� �Լ�
	bool IsPlayerExist();
	void DoRandomMove();
};