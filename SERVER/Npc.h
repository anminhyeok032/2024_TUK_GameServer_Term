#pragma once
#include "Session.h"

class Npc : public SESSION
{
public:
	std::atomic_bool active_;

public:
	Npc() : active_(false) {}
	~Npc() {}


	void DoReceive() override;
	void SendLoginInfoPacket() override;
	void DoSend(void* packet) override;
	void SendMovePacket(int c_id) override;
	void SendAddObjectPacket(int c_id) override;
	void SendRemoveObjectPacket(int c_id) override;
	void ProcessPacket(char* packet) override;

	SOCKET GetSocket() override;
	void SetSocket(SOCKET socket) override;

	void SetActive(bool active) { active_ = active; }
	bool GetActive() { return active_; }

	void DoRandomMove();
};