#pragma once
#include "Session.h"


class Player : public SESSION
{
public:
	SOCKET socket_;



	int		hp;
	int		max_hp;
	int		exp;
	int		level;

public:
	Player() : socket_(INVALID_SOCKET), hp(0), max_hp(0), exp(0), level(0) {}
	~Player() {}

	SOCKET GetSocket() { return socket_; };
	void SetSocket(SOCKET socket) { socket_ = socket; }

	void DoReceive() override;
	void SendLoginInfoPacket() override;
	void DoSend(void* packet) override;
	void SendMovePacket(int c_id) override;
	void SendAddObjectPacket(int c_id) override;
	void SendRemoveObjectPacket(int c_id) override;

	void ProcessPacket(char* packet) override;
};