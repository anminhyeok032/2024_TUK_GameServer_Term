#pragma once
#include "Session.h"

class Npc : public SESSION
{
public:
	std::atomic_bool active_;

public:
	Npc() : active_(false) {}
	~Npc() {}


	void DoReceive() {}
	void SendLoginInfoPacket() {}
	void DoSend(void* packet) {}
	void SendMovePacket(int c_id) {}
	void SendAddObjectPacket(int c_id) {}
	void SendRemoveObjectPacket(int c_id) {}
	void ProcessPacket(char* packet) {}

	SOCKET GetSocket() { return SOCKET(); }
	void SetSocket(SOCKET socket) {}
	void CloseSocket() {}

	void SetActive(bool active) { active_ = active; }
	bool GetActive() { return active_; }

	void DoRandomMove();
};