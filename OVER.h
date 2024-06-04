#pragma once
#include "global.h"
#include "protocol.h"

class OVER
{
public:
	WSAOVERLAPPED over_;
	WSABUF wsabuf_;
	char send_buf_[BUF_SIZE];
	COMP_KEY comp_key_;

	OVER()
	{
		wsabuf_.buf = send_buf_;
		wsabuf_.len = BUF_SIZE;
		comp_key_ = RECV;
		ZeroMemory(&over_, sizeof(over_));
	}
	OVER(char* packet)
	{
		wsabuf_.len = packet[0];
		wsabuf_.buf = send_buf_;
		ZeroMemory(&over_, sizeof(over_));
		comp_key_ = SEND;
		memcpy(send_buf_, packet, wsabuf_.len);
	}
};