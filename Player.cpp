#include "Player.h"

void Player::DoReceive()
{
	DWORD recv_flag = 0;
	memset(&recv_over_.over_, 0, sizeof(recv_over_.over_));
	recv_over_.wsabuf_.buf = prev_packet_.data() + prev_packet_.size();
	recv_over_.wsabuf_.len = BUF_SIZE - prev_packet_.size();
	WSARecv(socket_, &recv_over_.wsabuf_, 1, nullptr, &recv_flag, &recv_over_.over_, nullptr);
}

void Player::SendLoginInfoPacket()
{
}

void Player::DoSend(void* packet)
{
}

void Player::SendMovePacket(int c_id)
{
}

void Player::SendAddObjectPacket(int c_id)
{
}

void Player::SendRemoveObjectPacket(int c_id)
{
}

void Player::ProcessPacket(char* packet)
{
}

