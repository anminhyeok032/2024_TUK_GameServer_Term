//#include "protocol.h"
#include "global.h"
#include "OVER.h"
#include "Session.h"
#include "Player.h"

HANDLE g_h_iocp;

std::array<std::unique_ptr<SESSION>, MAX_NPC + MAX_USER> objects;

void Woker()
{
	while (true)
	{
		DWORD bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over;
		int ret = GetQueuedCompletionStatus(g_h_iocp, &bytes, &key, &over, INFINITE);
		OVER* ex_over = reinterpret_cast<OVER*>(over);
		if (false == ret)
		{
			if (ex_over->comp_key_ == ACCEPT)
			{
				std::cout << "Error : Accept" << std::endl;
			}
			else 
			{
				std::cout << "Error : GQCS error Client [" << key << "]" << std::endl;
				//disconnect(key);
				continue;
			}
		}

		if(bytes == 0)
		{
			std::cout << "Error : bytes == 0" << std::endl;
			//disconnect(key);
			continue;
		}

		switch (ex_over->comp_key_) {
		case ACCEPT:
		{
			break;
		}
		case RECV:
		{
			char* p = ex_over->send_buf_;
			int total_data = bytes + objects[key]->prev_packet_.size();

			auto& buffer = objects[key]->prev_packet_;
			buffer.insert(buffer.end(), ex_over->send_buf_, ex_over->send_buf_ + bytes);

			while (buffer.size() > 0)
			{
				int packet_size = static_cast<int>(p[0]);
				if (packet_size <= buffer.size())
				{
					objects[key]->ProcessPacket(buffer.data());
					buffer.erase(buffer.begin(), buffer.begin() + packet_size);
				}
				else
				{
					break;
				}
			}
			objects[key]->DoReceive();
			break;
		}
		case SEND:
		{
			delete ex_over;
			break;
		}
		}
	}
}

void InitializeNpc()
{
	for (int i = 0; i < MAX_NPC; i++)
	{
		objects[i] = std::make_unique<Player>();
		objects[i]->id_ = i;
		
	}
}

int main()
{
	// main start logic
	WSADATA WSADATA;
	WSAStartup(MAKEWORD(2, 2), &WSADATA);
	SOCKET server = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(server, reinterpret_cast<SOCKADDR*> (&server_addr), sizeof(server_addr));
	listen(server, SOMAXCONN);
	SOCKADDR_IN client_addr;
	int client_addr_size = sizeof(client_addr);
	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(server), g_h_iocp, 9999, 0);
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	OVER over;
	AcceptEx(server, c_socket, over.send_buf_, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &over.over_);

	std::vector <std::thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();

	// cpu 코어 개수만큼 woker 스레드 사용
	for (int i = 0; i < num_threads; i++)
	{
		worker_threads.emplace_back(Woker);
	}
	for (auto& th : worker_threads)
	{
		th.join();
	}
	closesocket(server);
	WSACleanup();
}
