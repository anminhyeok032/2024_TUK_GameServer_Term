//#include "protocol.h"
#include "global.h"
#include "OVER.h"
#include "Session.h"
#include "Player.h"
#include "Npc.h"


SOCKET g_server_socket, g_client_socket;
HANDLE g_h_iocp;
OVER g_over;
std::array<std::unique_ptr<SESSION>, MAX_NPC + MAX_USER> objects;




int get_new_client_id()
{
	for (int i = MAX_NPC; i < MAX_NPC + MAX_USER; ++i) 
	{
		if (objects[i])
		{
			std::lock_guard <std::mutex> ll{ objects[i]->mut_state_ };
			if (objects[i]->state_ == OS_FREE)
			{
				return i;
			}
		}
	}
	return -1;
}


void Woker()
{
	while (true)
	{
		DWORD bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(g_h_iocp, &bytes, &key, &over, INFINITE);
		OVER* ex_over = reinterpret_cast<OVER*>(over);
		if (FALSE == ret)
		{
			if (ex_over->comp_key_ == ACCEPT)
			{
				std::cout << "Error : Accept" << std::endl;
			}
			else 
			{
				std::cout << "Error : GQCS error Client [" << key << "]" << std::endl;
				//disconnect(key);
				if (ex_over->comp_key_ == SEND) delete ex_over;
				continue;
			}
		}
		if(bytes == 0)
		{
			if ((ex_over->comp_key_ == RECV) || (ex_over->comp_key_ == SEND))
			{
				std::cout << "Error : bytes == 0" << std::endl;
				//disconnect(key);
				continue;
			}
		}

		switch (ex_over->comp_key_) {
		case ACCEPT:
		{
			int client_id = get_new_client_id();
			if (client_id != -1)
			{
				{
					std::lock_guard<std::mutex> ll(objects[client_id]->mut_state_);
					objects[client_id]->state_ = OS_ACTIVE;
				}
				objects[client_id]->x_ = 0;
				objects[client_id]->y_ = 0;
				objects[client_id]->id_ = client_id;
				objects[client_id]->name_[0] = 0;
				objects[client_id]->prev_packet_.clear();
				objects[client_id]->SetSocket(g_client_socket);
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_client_socket),
					g_h_iocp, client_id, 0);
				objects[client_id]->DoReceive();

				// 다른 플레이어 위해 소켓 초기화
				g_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else
			{
				std::cout << "Error : Max User" << std::endl;
			}

			ZeroMemory(&g_over.over_, sizeof(g_over.over_));
			AcceptEx(g_server_socket, g_client_socket, g_over.send_buf_, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &g_over.over_);
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

void InitializeObjects()
{
	std::cout << "=====InitializeObjects Begin=====" << std::endl;
	//for (int i = 0; i < MAX_NPC; i++)
	//{
	//	objects[i] = std::make_unique<Npc>();
	//}
	for (int i = MAX_NPC; i < MAX_NPC + MAX_USER; ++i) 
	{
		objects[i] = std::make_unique<Player>();
	}
	std::cout << "=====InitializeObjects End=====" << std::endl;
}

int main()
{
	std::wcout.imbue(std::locale("korean"));
	// main start logic
	WSADATA WSADATA;
	WSAStartup(MAKEWORD(2, 2), &WSADATA);

	g_server_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(g_server_socket, reinterpret_cast<SOCKADDR*>(&server_addr), sizeof(server_addr));
	listen(g_server_socket, SOMAXCONN);
	SOCKADDR_IN client_addr;
	int client_addr_size = sizeof(client_addr);
	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_server_socket), g_h_iocp, 9999, 0);
	g_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_over.comp_key_ = ACCEPT;
	AcceptEx(g_server_socket, g_client_socket, g_over.send_buf_, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &g_over.over_);

	std::vector <std::thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();

	InitializeObjects();

	// cpu 코어 개수만큼 woker 스레드 사용
	for (int i = 0; i < num_threads; i++)
	{
		worker_threads.emplace_back(Woker);
	}
	for (auto& th : worker_threads)
	{
		th.join();
	}
	closesocket(g_server_socket);
	WSACleanup();
}
