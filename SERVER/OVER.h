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

	// ai_target_c_id_ : AI가 추적하는 대상의 client id
	int ai_target_c_id_;

	OVER()
	{
		wsabuf_.buf = send_buf_;
		wsabuf_.len = BUF_SIZE;
		comp_key_ = KEY_RECV;
		ZeroMemory(&over_, sizeof(over_));
	}
	OVER(char* packet)
	{
		wsabuf_.len = packet[0];
		wsabuf_.buf = send_buf_;
		ZeroMemory(&over_, sizeof(over_));
		comp_key_ = KEY_SEND;
		memcpy(send_buf_, packet, wsabuf_.len);
	}
};