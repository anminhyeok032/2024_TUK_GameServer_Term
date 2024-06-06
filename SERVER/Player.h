#pragma once
#include "Session.h"



class Player : public SESSION
{
public:
	SOCKET socket_;



	int		hp_;
	int		max_hp_;
	int		exp_;
	int		level_;

public:
	Player() : socket_(INVALID_SOCKET), hp_(0), max_hp_(0), exp_(0), level_(0) {}
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