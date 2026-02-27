#pragma once
#include "Session.h"

// 전방 선언
class Inventory; 
class RankingManager;
class Item;

class Player : public SESSION
{
public:
	SOCKET socket_;

	int		exp_;
	std::chrono::system_clock::time_point  last_action_time_;
	std::chrono::system_clock::time_point last_rank_req_time_;

	// 인벤토리 멤버
	Inventory* inventory_; 

public:
	// 생성자/소멸자 선언 (구현은 cpp)
	Player();
	~Player();

	void InitInventory();

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
	
	// 공격 패킷 인자 추가 (attack_type, direction, damage)
	void SendAttackPacket(int attacker_id, int damaged_id, int exp, char attack_type, char direction, int damage) override;
	
	void SendStatChangePacket() override;
	void SendGetItemPacket(Item* item);
	void SendInventorySyncPacket(); // 로그인 시 인벤토리 전체 동기화

	void DBLogin(SQLHDBC& hdbc) override;
	void DBLogout(SQLHDBC& hdbc) override;
	void SaveToRedis() override;

	// Inventory Redis Func
	void SaveInventoryToRedis();
	void LoadInventoryFromRedis();

	void ProcessPacket(char* packet) override;

	void DeleteFromRedis();
};