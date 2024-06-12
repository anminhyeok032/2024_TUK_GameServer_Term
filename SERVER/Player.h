#pragma once
#include "Session.h"



class Player : public SESSION
{
public:
	SOCKET socket_;

	int		exp_;
	int		level_;

public:
	Player() : socket_(INVALID_SOCKET), exp_(0), level_(0) {}
	~Player() {}

	SOCKET GetSocket() { return socket_; };
	void CloseSocket() { closesocket(socket_); socket_ = INVALID_SOCKET; }
	void SetSocket(SOCKET socket) { socket_ = socket; }

	void DoReceive() override;
	void SendLoginInfoPacket() override;
	void DoSend(void* packet) override;
	void SendMovePacket(int c_id) override;
	void SendAddObjectPacket(int c_id) override;
	void SendRemoveObjectPacket(int c_id) override;
	void SendChatPacket(int c_id, char mess[CHAT_SIZE]) override;
	void SendAttackPacket(int attacker_id, int damaged_id, int hp, bool alive) override;
	void SendStatChangePacket() override;

	void ProcessPacket(char* packet) override;
};