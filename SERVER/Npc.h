#pragma once
#include "Session.h"

class Npc : public SESSION
{
public:
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