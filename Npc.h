#pragma once
#include "Session.h"

class Npc : public SESSION
{
public:
	std::atomic_bool active_;

public:
	Npc() : active_(false) {}
	~Npc() {}

	/*void DoReceive() override;
	void SendLoginInfoPacket() override;
	void DoSend(void* packet) override;
	void SendMovePacket(int c_id) override;
	void SendAddObjectPacket(int c_id) override;
	void SendRemoveObjectPacket(int c_id) override;
	void ProcessPacket(char* packet) override;

	SOCKET GetSocket() override;
	void SetSocket(SOCKET socket) override;*/


	virtual void DoReceive() = 0;
	virtual void SendLoginInfoPacket() = 0;
	virtual void DoSend(void* packet) = 0;
	virtual void SendMovePacket(int c_id) = 0;
	virtual void SendAddObjectPacket(int c_id) = 0;
	virtual void SendRemoveObjectPacket(int c_id) = 0;
	virtual void ProcessPacket(char* packet) = 0;

	virtual SOCKET GetSocket() = 0;
	virtual void SetSocket(SOCKET socket) = 0;

	void DoRandomMove();
};