#pragma once
#include "Session.h"



class Player : public SESSION
{
public:
	SOCKET socket_;

	int		exp_;
	std::chrono::system_clock::time_point  last_action_time_;

public:
	Player() : socket_(INVALID_SOCKET), exp_(0) {}
	~Player() {}

	SOCKET GetSocket() { return socket_; };
	void CloseSocket() { closesocket(socket_); socket_ = INVALID_SOCKET; }
	void SetSocket(SOCKET socket) { socket_ = socket; }

	void DoReceive() override;
	void SendLoginInfoPacket() override;
	void SendLoginFailPacket() override;
	void DoSend(void* packet) override;
	void SendMovePacket(int c_id) override;
	void SendAddObjectPacket(int c_id) override;
	void SendRemoveObjectPacket(int c_id) override;
	void SendChatPacket(int c_id, char mess[CHAT_SIZE]) override;
	void SendAttackPacket(int attacker_id, int damaged_id, int exp) override;
	void SendStatChangePacket() override;

	void DBLogin(SQLHDBC& hdbc) override;
	void DBLogout(SQLHDBC& hdbc) override;

	void ProcessPacket(char* packet) override;
};