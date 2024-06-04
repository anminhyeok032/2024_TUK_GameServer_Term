#pragma once
#include "Session.h"


class Player : public SESSION
{
public:
	SOCKET socket_;
	char name_[NAME_SIZE];
public:
	Player() : socket_(INVALID_SOCKET), name_() {}
	~Player() {}

	void DoReceive() override;
	void SendLoginInfoPacket() override;
	void DoSend(void* packet) override;
	void SendMovePacket(int c_id) override;
	void SendAddObjectPacket(int c_id) override;
	void SendRemoveObjectPacket(int c_id) override;

	void ProcessPacket(char* packet) override;
};