#pragma once
#include "OVER.h"


class SESSION
{
public:
	// player�� npc�� ���� ��� ����
	OVER recv_over_;
	int id_;
	short x_, y_;

	// ���� ������ �丮��Ʈ
	std::pair<int, int> current_sector{ -99, -99 };
	std::unordered_set<int> view_list_;

	// mutex
	std::mutex mut_view_;

	// packet ������
	std::vector<char> prev_packet_;


public:
	SESSION() : id_(-1), x_(0), y_(0) {}
	~SESSION() {}

	virtual void DoReceive() = 0;
	virtual void SendLoginInfoPacket() = 0;
	virtual void DoSend(void* packet) = 0;
	virtual void SendMovePacket(int c_id) = 0;
	virtual void SendAddObjectPacket(int c_id) = 0;
	virtual void SendRemoveObjectPacket(int c_id) = 0;
	virtual void ProcessPacket(char* packet) = 0;
};

extern std::array<SESSION, MAX_USER> clients;