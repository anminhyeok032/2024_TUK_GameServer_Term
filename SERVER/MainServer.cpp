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
std::unordered_set<int> g_player_list;
std::mutex g_mut_player_list;
std::map <std::pair<int, int>, Sector> g_ObjectSector;
concurrency::concurrent_priority_queue<EVENT> g_event_queue;
concurrency::concurrent_queue<DBRequest> g_db_request_queue;

// 시야가 클라이언트에 맞춰 사각형의 형태이다
bool CanSee(int a, int b)
{
	int dx = std::abs(objects[a]->x_ - objects[b]->x_);
	int dy = std::abs(objects[a]->y_ - objects[b]->y_);
	return (dx <= VIEW_RANGE/2) && (dy <= VIEW_RANGE/2);
}

bool IsNpc(int a)
{
	return a < MAX_NPC;
}
bool IsPlayer(int a)
{
	return a >= MAX_NPC;
}


int GetNewClientId()
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
		else
		{
			objects[i] = std::make_unique<Player>();
			return i;
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
			if (ex_over->comp_key_ == KEY_ACCEPT)
			{
				//std::cout << "Error : Accept" << std::endl;
				disconnect(key);
			}
			else 
			{
				//std::cout << "Error : GQCS error Client [" << key << "]" << std::endl;
				disconnect(key);
				if (ex_over->comp_key_ == KEY_SEND) delete ex_over;
				continue;
			}
		}
		if(bytes == 0)
		{
			if ((ex_over->comp_key_ == KEY_RECV) || (ex_over->comp_key_ == KEY_SEND))
			{
				//std::cout << "Error : Client [" << key << "]" << std::endl;
				disconnect(key);
				if (ex_over->comp_key_ == KEY_SEND) delete ex_over;
				continue;
			}
		}

		switch (ex_over->comp_key_) {
		case KEY_ACCEPT:
		{
			int client_id = GetNewClientId();
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
				objects[client_id]->visual_ = 0;
				objects[client_id]->SetSocket(g_client_socket);
				objects[client_id]->PutInSector();
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_client_socket),
					g_h_iocp, client_id, 0);
				objects[client_id]->DoReceive();
				// 접속 플레이어 리스트에 저장
				g_mut_player_list.lock();
				g_player_list.insert(client_id);
				g_mut_player_list.unlock();
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
		case KEY_RECV:
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
		case KEY_SEND:
		{
			delete ex_over;
			break;
		}

		//=======================================================
		// AI 처리
		//=======================================================
		case KEY_NPC_RANDOM_MOVE:
		{
			objects[key]->DoRandomMove(ex_over->ai_target_c_id_);
			delete ex_over;
			break;
		}
		}
	}
}

void disconnect(int c_id)
{

	for (auto& sector : objects[c_id]->around_sector_)
	{
		for (auto& id : g_ObjectSector[sector].sec_id_)
		{
			{
				std::lock_guard<std::mutex> ll(objects[id]->mut_state_);
				if (OS_INGAME != objects[id]->state_) continue;
			}
			if (objects[id]->id_ == c_id) continue;
			if (IsNpc(objects[id]->id_)) continue;
			// 눈에 보이는 애들에게만 보냄
			if (false == CanSee(objects[id]->id_, c_id)) continue;
			objects[id]->SendRemoveObjectPacket(c_id);
		}
	}
	objects[c_id]->CloseSocket();
	g_db_request_queue.push({ DBRequest::LOGOUT, c_id });

	{
		std::lock_guard<std::mutex> ll(objects[c_id]->mut_state_);
		objects[c_id]->state_ = OS_FREE;
	}
	objects[c_id]->current_sector_ = { -99, -99 };
	objects[c_id]->around_sector_.clear();

	g_mut_player_list.lock();
	g_player_list.erase(c_id);
	g_mut_player_list.unlock();
	
	// 섹터에서 로그아웃한 id 삭제
	for (auto& sector : g_ObjectSector)
	{
		std::lock_guard<std::mutex> sec_l(sector.second.mut_sector_);
		{
			if (sector.second.sec_id_.find(c_id) != sector.second.sec_id_.end()) {
				// 기존 섹터에서 플레이어 정보를 삭제
				sector.second.sec_id_.erase(c_id);
				break;
			}
		}
	}
}

void InitializeObjects()
{
	std::cout << "===== Initialize NPC Begin =====" << std::endl;
	for (int i = 0; i < MAX_NPC; i++)
	{
		objects[i] = std::make_unique<Npc>();
		objects[i]->id_ = i;
		if (i > MAX_NPC / 2)
		{
			sprintf_s(objects[i]->name_, "Agro %d", i);
			objects[i]->visual_ = 1;
		}
		else
		{
			sprintf_s(objects[i]->name_, "Peace %d", i);
			objects[i]->visual_ = 2;
		}
		objects[i]->x_ = rand() % W_WIDTH;
		objects[i]->y_ = rand() % W_HEIGHT;
		objects[i]->SetStartPos(objects[i]->x_, objects[i]->y_);
		objects[i]->state_ = OS_INGAME;
		objects[i]->level_ = rand() % 10 + 1;
		objects[i]->max_hp_ = objects[i]->level_ * 10;
		objects[i]->hp_ = objects[i]->max_hp_;
		objects[i]->SetActive(false);

		std::pair<int, int> new_sector = { objects[i]->x_ / SEC_ROW, objects[i]->y_ / SEC_COL };
		objects[i]->current_sector_ = new_sector;
		g_ObjectSector[new_sector].sec_id_.insert(objects[i]->id_);
		objects[i]->PutInSector();
	}

	std::cout << "===== Initialize NPC End =====" << std::endl;
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
	g_over.comp_key_ = KEY_ACCEPT;
	AcceptEx(g_server_socket, g_client_socket, g_over.send_buf_, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &g_over.over_);

	std::vector <std::thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();

	InitializeObjects();

	// DB 스레드 생성
	SQLHDBC hdbc = ConnectWithDataBase();
	std::thread db_thread(DBWoker, hdbc);
	//ai 스레드 생성
	//std::thread ai_thread(DoAITimer);

	// cpu 코어 개수만큼 woker 스레드 사용
	for (int i = 0; i < num_threads; i++)
	{
		worker_threads.emplace_back(Woker);
	}
	for (auto& th : worker_threads)
	{
		th.join();
	}
	db_thread.join();
	//ai_thread.join();
	closesocket(g_server_socket);
	WSACleanup();
}
