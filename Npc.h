#pragma once
#include "Session.h"

class Npc : public SESSION
{
public:
	std::atomic_bool active_;
	char name_[NAME_SIZE];
public:
	Npc() : name_() {}
	~Npc() {}

	/*void DoReceive() override;
	void SendLoginInfoPacket() override;
	void DoSend(void* packet) override;
	void SendMovePacket(int c_id) override;
	void SendAddObjectPacket(int c_id) override;
	void SendRemoveObjectPacket(int c_id) override;*/
};