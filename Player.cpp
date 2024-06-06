#include "Player.h"

void print_error(const char* msg, int err_no)
{
	WCHAR* msg_buf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&msg_buf), 0, NULL);
	std::cout << msg;
	std::wcout << L" : 에러 : " << msg_buf;
	while (true);
	LocalFree(msg_buf);
}

void Player::DoReceive()
{
	DWORD recv_flag = 0;
	memset(&recv_over_.over_, 0, sizeof(recv_over_.over_));
	recv_over_.wsabuf_.buf = recv_over_.send_buf_ + prev_packet_.size();
	recv_over_.wsabuf_.len = BUF_SIZE - prev_packet_.size();
	int res = WSARecv(socket_, &recv_over_.wsabuf_, 1, 0, &recv_flag, 
		&recv_over_.over_, 0);
	if(res == SOCKET_ERROR)
	{
		int err_no = WSAGetLastError();
		if(err_no != WSA_IO_PENDING)
		{
			print_error("WSARecv", err_no);
		}
	}
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
	switch (packet[1])
	{
		// 로그인 패킷 처리
		case CS_LOGIN:
		{
			CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
			strcpy_s(name_, p->name);
			std::cout << "Login : " << name_ << std::endl;
			break;
		}
	}
}

