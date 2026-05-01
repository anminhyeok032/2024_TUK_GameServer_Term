#include "Player.h"
#include "Npc.h"
#include "RankingManager.h"
#include "Inventory.h" 
#include "MapItem.h"

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

Player::Player() : socket_(INVALID_SOCKET), exp_(0), inventory_(nullptr)
{
	//inventory_ = new Inventory(id_);
}

Player::~Player()
{
	if (inventory_) delete inventory_;
}

void Player::InitInventory()
{
	if (inventory_) delete inventory_;
	inventory_ = new Inventory(id_);
}

void Player::DoReceive()
{
	DWORD recv_flag = 0;
	memset(&recv_over_.over_, 0, sizeof(recv_over_.over_));

	recv_over_.wsabuf_.buf = recv_over_.send_buf_;
	recv_over_.wsabuf_.len = BUF_SIZE;

	WSARecv(socket_, &recv_over_.wsabuf_, 1, 0, &recv_flag,
		&recv_over_.over_, 0);
	
}

void Player::SendLoginInfoPacket()
{
	SC_LOGIN_INFO_PACKET packet;
	packet.id = id_;
	packet.size = sizeof(SC_LOGIN_INFO_PACKET);
	packet.type = SC_LOGIN_INFO;
	packet.exp = exp_;
	packet.hp = hp_;
	packet.level = level_;
	packet.max_hp = max_hp_;
	packet.x = x_;
	packet.y = y_;
	packet.visual = OT_PLAYER;
	DoSend(&packet);
}

void Player::SendLoginFailPacket()
{
	SC_LOGIN_FAIL_PACKET packet;
	packet.size = sizeof(SC_LOGIN_FAIL_PACKET);
	packet.type = SC_LOGIN_FAIL;
	DoSend(&packet);
}

void Player::DoSend(void* packet)
{
	unsigned short pkt_size = reinterpret_cast<unsigned short*>(packet)[0];

	// 버퍼 크기보다 패킷의 크기가 클 경우 전송 중단
	if (pkt_size > BUF_SIZE)
	{
		std::cout << "[CRITICAL] Packet Size(" << pkt_size
			<< ") is larger than BUF_SIZE(" << BUF_SIZE << ")!" << std::endl;
		return;
	}

	OVER* sdata = g_sendPool.Acquire();
	memcpy(sdata->send_buf_, packet, pkt_size);

	sdata->wsabuf_.buf = sdata->send_buf_;
	sdata->wsabuf_.len = pkt_size;
	sdata->comp_key_ = KEY_SEND;
	ZeroMemory(&sdata->over_, sizeof(sdata->over_));

	// WSASend 에러 체크 추가
	if (WSASend(socket_, &sdata->wsabuf_, 1, 0, 0, &sdata->over_, 0) == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			// 클라이언트 종료 등으로 인해 소켓이 닫힌 정상적인 상황의 에러는 출력하지 않음
			if (err != WSAENOTSOCK && err != WSAECONNRESET && err != WSAECONNABORTED)
			{
				std::cout << "[Error] WSASend Failed: " << err << std::endl;
			}
			g_sendPool.Release(sdata);
		}
	}
}

void Player::SendMovePacket(int c_id)
{
	SC_MOVE_OBJECT_PACKET packet;
	packet.size = sizeof(SC_MOVE_OBJECT_PACKET);
	packet.type = SC_MOVE_OBJECT;
	packet.id = c_id;
	packet.x = objects[c_id]->x_;
	packet.y = objects[c_id]->y_;
	packet.move_time = objects[c_id]->last_move_time_;
	DoSend(&packet);
}

void Player::SendAddObjectPacket(int c_id)
{
	// 자신의 뷰리스트에 추가
	mut_view_.lock();
	view_list_.insert(c_id);
	mut_view_.unlock();

	// MapItem인 경우 다른 패킷 전송
	if (true == IsMapItem(c_id)) 
	{
		objects[c_id]->SendAddObjectPacket(id_); // MapItem이 직접 패킷 만듦
		return;
	}

	SC_ADD_OBJECT_PACKET packet;
	packet.size = sizeof(SC_ADD_OBJECT_PACKET);
	packet.type = SC_ADD_OBJECT;
	packet.id = c_id;
	packet.x = objects[c_id]->x_;
	packet.y = objects[c_id]->y_;
	packet.visual = objects[c_id]->visual_;
	strcpy_s(packet.name, objects[c_id]->name_);
	DoSend(&packet);
}

void Player::SendRemoveObjectPacket(int c_id)
{
	mut_view_.lock();
	view_list_.erase(c_id);
	mut_view_.unlock();

	// MapItem인 경우 다른 패킷 전송
	if (true == IsMapItem(c_id)) 
	{
		objects[c_id]->SendRemoveObjectPacket(id_);
		return;
	}

	SC_REMOVE_OBJECT_PACKET packet;
	packet.size = sizeof(SC_REMOVE_OBJECT_PACKET);
	packet.type = SC_REMOVE_OBJECT;
	packet.id = c_id;
	DoSend(&packet);
}

void Player::SendChatPacket(int c_id, char mess[CHAT_SIZE])
{
	SC_CHAT_PACKET packet;
	packet.size = sizeof(SC_CHAT_PACKET);
	packet.type = SC_CHAT;
	packet.id = c_id;
	strcpy_s(packet.mess, mess);
	DoSend(&packet);
}

void Player::SendStatChangePacket()
{
	SC_STAT_CHANGE_PACKET packet;
	packet.size = sizeof(SC_STAT_CHANGE_PACKET);
	packet.type = SC_STAT_CHANGE;
	packet.hp = hp_;
	packet.max_hp = max_hp_;
	packet.exp = exp_;
	packet.level = level_;
	DoSend(&packet);
}

void Player::SendGetItemPacket(Item* item)
{
	SC_GET_ITEM_PACKET packet;
	packet.size = sizeof(SC_GET_ITEM_PACKET);
	packet.type = SC_GET_ITEM;
	packet.item_uid = item->item_uid;
	packet.template_id = item->template_id;
	packet.x = item->x;
	packet.y = item->y;
	packet.is_rotated = item->is_rotated;
	DoSend(&packet);
}

void Player::SendInventorySyncPacket()
{
	if (!inventory_) return;

	auto inv_data = inventory_->GetInventoryDataForRedis();
	int item_count = static_cast<int>(inv_data.size());

	// 가변 크기 패킷 동적 할당
	// 실제 size = 헤더(size+type+count) + InventorySlot * item_count
	int base_size = offsetof(SC_INVENTORY_SYNC_PACKET, items); // size + type + item_count
	int total_size = base_size + sizeof(InventorySlot) * item_count;

	std::vector<char> buf(total_size, 0);
	SC_INVENTORY_SYNC_PACKET* packet = reinterpret_cast<SC_INVENTORY_SYNC_PACKET*>(buf.data());
	packet->size = static_cast<unsigned short>(total_size);
	packet->type = SC_INVENTORY_SYNC;
	packet->item_count = item_count;

	int idx = 0;
	for (auto& pair : inv_data)
	{
		int tid, x, y, rot;
		if (sscanf_s(pair.second.c_str(), "%d:%d:%d:%d", &tid, &x, &y, &rot) == 4)
		{
			InventorySlot& slot = packet->items[idx++];
			slot.item_uid   = std::stoll(pair.first);
			slot.template_id = tid;
			slot.x           = (short)x;
			slot.y           = (short)y;
			slot.is_rotated  = (bool)rot;
		}
	}
	// 실제 파싱된 개수로 보정
	packet->item_count = idx;
	packet->size = static_cast<unsigned short>(base_size + sizeof(InventorySlot) * idx);

	DoSend(packet);
}

void Player::DBLogin(SQLHDBC& hdbc)
{
	SQLHSTMT hstmt = AllocateStatement(hdbc);
	SQLRETURN retcode;

	// Stored Procedure 호출 준비
	retcode = SQLPrepare(hstmt, (SQLWCHAR*)L"{CALL sp_UserLogin(?)}", SQL_NTS);
	if (!(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO))
	{
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		SendLoginFailPacket();
		return;
	}

	// 파라미터 바인딩 (user_id)
	SQLWCHAR dId[NAME_LEN];
	std::wstring wuser_id(name_, name_ + strlen(name_));
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
		NAME_LEN, 0, (SQLPOINTER)wuser_id.c_str(),
		wuser_id.size() * sizeof(wchar_t), nullptr);

	// 실행
	retcode = SQLExecute(hstmt);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		SendLoginFailPacket();
		return;
	}

	// 결과 바인딩
	SQLSMALLINT d_x, d_y, d_max_hp, d_level, d_visual;
	SQLINTEGER d_exp;
	SQLLEN cbId = 0, cb_x = 0, cb_y = 0, cb_max_hp = 0, cb_exp = 0, cb_level = 0, cb_visual = 0;

	SQLBindCol(hstmt, 1, SQL_C_WCHAR, dId, NAME_LEN * sizeof(SQLWCHAR), &cbId);
	SQLBindCol(hstmt, 2, SQL_C_SSHORT, &d_x, 0, &cb_x);
	SQLBindCol(hstmt, 3, SQL_C_SSHORT, &d_y, 0, &cb_y);
	SQLBindCol(hstmt, 4, SQL_C_SSHORT, &d_max_hp, 0, &cb_max_hp);
	SQLBindCol(hstmt, 5, SQL_C_SLONG, &d_exp, 0, &cb_exp);
	SQLBindCol(hstmt, 6, SQL_C_SSHORT, &d_level, 0, &cb_level);
	SQLBindCol(hstmt, 7, SQL_C_SSHORT, &d_visual, 0, &cb_visual);

	
	// 현재 해당 캐릭터가 접속중인지 확인후, 접속중이라면 중복접속 방지
	for (auto player : g_player_list)
	{
		if (player == id_) continue;
		if (0 == strncmp(name_, objects[player]->name_, sizeof(name_)))
		{
			std::wcerr << L"Login Failed: Already logged in." << std::endl;
				
			SendLoginFailPacket();
			SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
			name_[0] = 0; // 로그인 실패 처리: 로그아웃 시 데이터 저장 방지
			return;
		}
	}

	retcode = SQLFetch(hstmt);
	if (retcode == SQL_SUCCESS)
	{
		//wprintf(L"Login Success : User ID: %s, Location X: %d, Location Y: %d\n", dId, d_x, d_y);
		// DB에서 받은 정보 초기화
		x_ = d_x;
		y_ = d_y;
		max_hp_ = d_max_hp;
		hp_ = max_hp_;
		exp_ = d_exp;
		level_ = d_level;
		visual_ = d_visual;
		
		// 서버 크래시시, redis에 정보가 더 최신이면 덮어씌워 주기
		if (g_redis_client->is_connected())
		{
			std::string key(name_);
			key = "User:" + key;

			auto future_reply = g_redis_client->hgetall(key);
			g_redis_client->sync_commit();

			auto reply = future_reply.get();
			if (reply.is_array() && reply.as_array().size() > 0)
			{
				std::cout << "[Recovery] Redis에서 최신 데이터로 복구 : " << name_ << std::endl;

				auto arr = reply.as_array();
				for (size_t i = 0; i < arr.size(); i += 2) {
					std::string field = arr[i].as_string();
					std::string value = arr[i + 1].as_string();

					// SQL 데이터보다 Redis 데이터가 최신이므로 덮어씀
					if (field == "x") x_ = (short)std::stoi(value);
					else if (field == "y") y_ = (short)std::stoi(value);
					else if (field == "hp") hp_ = std::stoi(value);
					else if (field == "exp") exp_ = std::stoi(value);
					else if (field == "level") level_ = std::stoi(value);
				}
			}
			// 인벤토리 로드
			LoadInventoryFromRedis();
		}

		// Redis에 인벤토리가 없으면 SQL에서 로드 (폴백)
		if (inventory_ && inventory_->GetAllItems().empty())
		{
			DBLoadInventory(hdbc);
		}

		// 로그인 성공 정보를 Redis에도 갱신
		SaveToRedis();

		//===============
		// Login 패킷 전송
		//===============
		SendLoginInfoPacket();

		// 인벤토리 전체를 패킷 1개로 한 번에 동기화
		SendInventorySyncPacket();

		// 자신의 위치 섹터에 넣기
		PutInSector();
		// 해당 객체 INGAME 상태로 변경
		{
			std::lock_guard<std::mutex> lock(mut_state_);
			state_ = OS_INGAME;
		}

		for (auto& sector : around_sector_)
		{
			{
				// 섹터에 대해 lock
				std::lock_guard<std::mutex> sec_l(g_ObjectSector[sector].mut_sector_);
				for (auto& id : g_ObjectSector[sector].sec_id_)
				{
					{
						std::lock_guard<std::mutex> ll(objects[id]->mut_state_);
						if (OS_INGAME != objects[id]->state_) continue;
					}

					if (false == CanSee(id_, objects[id]->id_))	continue;
					if (objects[id]->id_ == id_)	continue;	// 자기자신일때
					
					// MapItem 처리 추가
					if (true == IsMapItem(objects[id]->id_)) 
					{
						// 내 view_list_에 MapItem 추가 후 패킷 전송
						mut_view_.lock();
						view_list_.insert(objects[id]->id_);
						mut_view_.unlock();
						objects[id]->SendAddObjectPacket(id_); // MapItem -> Me
					} 
					else 
					{
						// NPC일 경우 접속한 플레이어를 인식하고 바로 깨어나도록 처리
						if (IsNpc(objects[id]->id_))
						{
							Npc* npc = dynamic_cast<Npc*>(objects[id].get());
							if (npc)
							{
								npc->WakeUpNpc(id_);
							}
						}
						objects[id]->SendAddObjectPacket(id_); // Other -> Me
					}
					
					SendAddObjectPacket(objects[id]->id_); // Me -> Other
				}
			}
		}
	}
	else
	{
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
		name_[0] = { 0, };
		SendLoginFailPacket();
	}

	if (hstmt) 
	{
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	}
}

void Player::DBLogout(SQLHDBC& hdbc)
{
	// 로그인에 실패했거나 접속만 하고 패킷을 보내지 않은 클라이언트는 무시
	if (name_[0] == 0) return;

	SQLHSTMT hstmt = AllocateStatement(hdbc);
	SQLRETURN retcode;

	std::wstring w_user_id(name_, name_ + strlen(name_));

	// 준비된 문으로 Stored Procedure 호출
	retcode = SQLPrepare(hstmt, (SQLWCHAR*)L"{CALL sp_UserLogout(?, ?, ?, ?, ?, ?)}", SQL_NTS);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		return;
	}

	// 파라미터 바인딩
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, w_user_id.size(), 0,
		(SQLWCHAR*)w_user_id.c_str(), 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0,
		&x_, 0, NULL);
	SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0,
		&y_, 0, NULL);
	SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0,
		&max_hp_, 0, NULL);
	SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
		&exp_, 0, NULL);
	SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0,
		&level_, 0, NULL);

	// 실행
	retcode = SQLExecute(hstmt);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		std::wcout << "[" << name_ << L"] : Logout successful, data saved. "
			<< L"x = " << x_ << L", y = " << y_ << std::endl;


		// Redis에서 데이터를 영구저장 안함 - 패킷 받을 때 조회를 위해
		// 접속중 상태만 갱신하고 TTL을 7일로 재설정함 -> 7일 초과시 삭제
		SaveToRedis();

		DBSaveInventory(hdbc);

		name_[0] = 0;
	}
	else
	{
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
	}

	SQLFreeHandle(SQL_HANDLE_STMT, hdbc);
}

void Player::SendAttackPacket(int attacker_id, int damaged_id, int exp, char attack_type, char direction, int damage)
{
	SC_ATTACK_PACKET packet;
	packet.size = sizeof(SC_ATTACK_PACKET);
	packet.type = SC_ATTACK;
	packet.attacker_id = attacker_id;
	packet.damaged_id = damaged_id;
	packet.max_hp = objects[damaged_id]->max_hp_;
	packet.hp = objects[damaged_id]->hp_;
	packet.exp = exp;
	packet.damage = damage;
	packet.attack_type = attack_type;
	packet.direction = direction;
	packet.center_x = objects[attacker_id]->x_;
	packet.center_y = objects[attacker_id]->y_;

	DoSend(&packet);
}


// 빈 맵 아이템 슬롯 찾기
int GetNewMapItemId() 
{
	// MAX_NPC + MAX_USER 부터 MAX_OBJECTS 사이의 빈 슬롯 찾기
	for (int i = MAX_NPC + MAX_USER; i < MAX_OBJECTS; ++i)
	{
		if (objects[i] == nullptr) 
		{
			objects[i] = std::make_unique<MapItem>();
			return i;
		}
		// 재사용 로직: state_ == OS_FREE인 것 찾기 (unique_ptr이므로 nullptr 체크 후 state 체크)
		else
		{
			std::lock_guard<std::mutex> ll(objects[i]->mut_state_);
			if (objects[i]->state_ == OS_FREE)
			{
				objects[i]->view_list_.clear();
				objects[i]->current_sector_ = { -99, -99 };
				objects[i]->around_sector_.clear();
				return i;
			}
		}
	}
	return -1;
}


void Player::ProcessPacket(char* packet)
{
	switch (packet[2])
	{
		// 로그인 패킷 처리
		case CS_LOGIN:
		{
			CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
			
			// 클라이언트가 강제로 빈 문자열 아이디를 보내는 경우 (비정상 접근 차단)
			if (p->name[0] == '\0')
			{
				SendLoginFailPacket();
				return;
			}

			strcpy_s(name_, p->name);
			last_action_time_ = std::chrono::system_clock::now();
			g_db_request_queue.push({ DBRequest::LOGIN, id_ });

			break;
		}
		// 이동 패킷 처리
		case CS_MOVE:
		{
			CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
			last_move_time_ = p->move_time;
			if (last_action_time_ + std::chrono::seconds(1) > std::chrono::system_clock::now())
				break;
			last_action_time_ = std::chrono::system_clock::now();
			short x = x_;
			short y = y_;
			switch (p->direction) {
			case 0: if (y > 0) y--; break;
			case 1: if (y < W_HEIGHT - 1) y++; break;
			case 2: if (x > 0) x--; break;
			case 3: if (x < W_WIDTH - 1) x++; break;
			}
			// TODO : 장애물 체크 해야함
			x_ = x;
			y_ = y;

			// Sector 이동
			PutInSector();

			// 기존 시야에 있는 플레이어 목록과 시야에 있는 플레이어 셋
			mut_view_.lock();
			std::unordered_set<int> prev_viewlist = view_list_;
			mut_view_.unlock();
			std::unordered_set<int> curr_viewlist;

			// 자신의 around_sector에 있는 object가 시야에 보이는지 검사->curr_viewlist에 삽입
			for (auto& sector : around_sector_)
			{
				{
					// 섹터에 대해 lock
					std::lock_guard<std::mutex> sec_l(g_ObjectSector[sector].mut_sector_);
					for (auto& id : g_ObjectSector[sector].sec_id_)
					{
						{
							std::lock_guard<std::mutex> ll(objects[id]->mut_state_);
							if (OS_INGAME != objects[id]->state_) continue;
						}

						if (false == CanSee(id_, objects[id]->id_))	continue;
						if (objects[id]->id_ == id_)	continue;	// 자기자신일때
						// NPC일 경우 WakeUp 호출 검사
						if (IsNpc(objects[id]->id_))
						{
							// objects 배열은 부모 클래스(Session *) 포인터라 캐스팅 필요
							Npc* npc = dynamic_cast<Npc*>(objects[id].get());
							if (npc) {
								npc->WakeUpNpc(id_);
							}
						}
						curr_viewlist.insert(objects[id]->id_);

					}
				}
			}
			// 자신에게 이동 전송
			SendMovePacket(id_);
			// 검사한 시야를 이용해서 새 오브젝트와 이동
			for (int ano_id : curr_viewlist)
			{
				if (0 == prev_viewlist.count(ano_id))
				{
					// MapItem 처리
					if (true == IsMapItem(ano_id)) 
					{
						// 내 view_list_에 MapItem 추가 후 패킷 전송
						mut_view_.lock();
						view_list_.insert(ano_id);
						mut_view_.unlock();
						objects[ano_id]->SendAddObjectPacket(id_);
					} 
					else 
					{
						SendAddObjectPacket(ano_id);
						objects[ano_id]->SendAddObjectPacket(id_);
					}
				}
				else
				{
					if (false == IsMapItem(ano_id)) objects[ano_id]->SendMovePacket(id_);
				}
			}
			for (int ano_id : prev_viewlist)
			{
				if (0 == curr_viewlist.count(ano_id))
				{
					// MapItem 처리
					if (true == IsMapItem(ano_id))
					{
						// 내 view_list_에서 MapItem 제거 후 패킷 전송
						mut_view_.lock();
						view_list_.erase(ano_id);
						mut_view_.unlock();
						objects[ano_id]->SendRemoveObjectPacket(id_);
					} 
					else
					{
						bool is_ingame = false;
						{
							std::lock_guard<std::mutex> ll(objects[ano_id]->mut_state_);
							is_ingame = (OS_INGAME == objects[ano_id]->state_);
						}

						if (is_ingame)
						{
							objects[ano_id]->SendRemoveObjectPacket(id_);
						}
						SendRemoveObjectPacket(ano_id);
					}
				}
			}

			break;
		}
		case CS_TELEPORT:
		{
			x_ = rand() % W_WIDTH;
			y_ = rand() % W_HEIGHT;
			break;
		}
		// 채팅 패킷 처리
		case CS_CHAT:
		{
			CS_CHAT_PACKET* p = reinterpret_cast<CS_CHAT_PACKET*>(packet);
			std::cout << "Chat : [" << name_ << "] : " << p->mess << std::endl;
			
			for (auto& player : g_player_list)
			{
				if (player == id_) continue;
				objects[player]->SendChatPacket(id_, p->mess);
			}
			break;
		}
		// 공격처리
		case CS_ATTACK:
		{
			CS_ATTACK_PACKET* p = reinterpret_cast<CS_ATTACK_PACKET*>(packet);

			if (last_action_time_ + std::chrono::seconds(1) > std::chrono::system_clock::now())
				break;
			last_action_time_ = std::chrono::system_clock::now();

			std::vector<std::pair<short, short>> attack_coord;
			short attack_x = x_;
			short attack_y = y_;

			// AttackType enum class로 타입 캐스팅
			AttackType attack_type = static_cast<AttackType>(p->attack_type);

			// 공격 타입별 데미지 계산
			int damage = 0;
			switch (attack_type)
			{
			case AttackType::NORMAL:
				damage = level_ * 3; // 단방향 평타: 레벨 * 3
				break;
			case AttackType::AOE:
				damage = level_ * 2; // 범위 공격: 레벨 * 2
				break;
			default:
				damage = level_ * 3; // 일단 알 수 없는 타입은 평타 취급
				break;
			}

			switch (p->attack_direction) {
			case 0:		// UP
				if (attack_y > 0)
				{
					attack_y--;
					attack_coord.emplace_back(attack_x, attack_y);
				}
				break;
			case 1:		// DOWN
				if (attack_y < W_HEIGHT - 1)
				{
					attack_y++;
					attack_coord.emplace_back(attack_x, attack_y);
				}
				break;
			case 2:		// LEFT
				if (attack_x > 0)
				{
					attack_x--;
					attack_coord.emplace_back(attack_x, attack_y);
				}
				break;
			case 3:		// RIGHT
				if (attack_x < W_WIDTH - 1)
				{
					attack_x++;
					attack_coord.emplace_back(attack_x, attack_y);
				}
				break;
			case 4:		// 4방향 공격
			{
				const std::array<std::pair<short, short>, 4> directions = 
				{
				std::make_pair(0, -1),  // UP
				std::make_pair(0, 1),   // DOWN
				std::make_pair(-1, 0),  // LEFT
				std::make_pair(1, 0)    // RIGHT
				};
				for (const auto& direction : directions)
				{
					short x = attack_x + direction.first;
					short y = attack_y + direction.second;

					if (x >= 0 && x < W_WIDTH && y >= 0 && y < W_HEIGHT) 
					{
						attack_coord.emplace_back(x, y);
					}
				}
				break;
			}
			}
			
			// 공격 적용
			for(const auto& coord : attack_coord)
			{
				std::vector<int> dead_npc_ids; // 사망한 NPC ID 수집용 (좌표마다 초기화)

				// 공격 위치에 맞는 섹터를 얻어서 검사함
				std::pair<int, int> sector_key = { coord.first / SEC_ROW, coord.second / SEC_COL };
				auto& sector = g_ObjectSector[sector_key];
				{
					std::lock_guard<std::mutex> lock(sector.mut_sector_);
					for (auto& id : sector.sec_id_)
					{
					if (id == id_) continue;
					if (true == IsPlayer(id)) continue;
					if (true == IsMapItem(id)) continue;

						{
							std::lock_guard<std::mutex> ll(objects[id]->mut_state_);
							if (OS_INGAME != objects[id]->state_) continue;
						}

						if (objects[id]->x_ == coord.first && objects[id]->y_ == coord.second)
						{
							// 공격 성공
							objects[id]->hp_ -= damage;

							if (objects[id]->hp_ <= 0)
							{
								// 사망
								objects[id]->hp_ = 0;
								objects[id]->state_ = OS_DEAD;

								dead_npc_ids.push_back(id);

								int getting_exp = objects[id]->level_ * objects[id]->level_ * 2;
								exp_ += getting_exp;
								int required_exp = static_cast<int>(100 * pow(2, level_ - 1));
								if (exp_ >= required_exp)
								{
									level_++;
									exp_ -= required_exp;
									std::cout << "Level up - " << level_ << "!" << std::endl;
									g_db_request_queue.push({ DBRequest::SAVE_REDIS, id_ });
								}
								SendStatChangePacket();

								// 사망 패킷 + 제거 처리 (damage는 0 - 이미 죽었으므로)
								for (auto& view_list : objects[id]->view_list_)
								{
									objects[view_list]->SendAttackPacket(id_, objects[id]->id_, getting_exp, p->attack_type, p->attack_direction, 0);
									objects[view_list]->SendRemoveObjectPacket(id);
								}
							}
							else
							{
								// 피격 객체 생존시 데미지 패킷 전송 (실제 데미지 전달)
								for (auto& view_list : objects[id]->view_list_)
								{
									objects[view_list]->SendAttackPacket(id_, objects[id]->id_, 0, p->attack_type, p->attack_direction, damage);
								}
							}
						}
					}
				}

				// 섹터 락 해제 후 아이템 드롭 처리
				for (int npc_id : dead_npc_ids)
				{
					if (true == IsNpc(npc_id))
					{
						Npc* npc = dynamic_cast<Npc*>(objects[npc_id].get());
						if (npc) npc->DropItem();
					}
				}
			}
			break;
		}
		case CS_RANKING_REQ:
		{
			// 쿨타임 체크 (3초) - 모두다 요청
			auto now = std::chrono::system_clock::now();
			if (now - last_rank_req_time_ < std::chrono::seconds(3)) break;
			last_rank_req_time_ = now;

			// 매니저에게 전송 요청 (DB/Redis 조회 없이, 메모리에서 바로 줌)
			RankingManager::GetInstance()->SendRankingToPlayer(id_);
			break;
		}
		// 아이템 이동 패킷 처리
		case CS_ITEM_MOVE:
		{
			CS_ITEM_MOVE_PACKET* p = reinterpret_cast<CS_ITEM_MOVE_PACKET*>(packet);
			bool success = inventory_->MoveItem(p->item_uid, p->new_x, p->new_y, p->is_rotated);
			
			if (success) SaveInventoryToRedis();
			break;
		}
		// 아이템 정렬
		case CS_ITEM_SORT:
		{
			CS_ITEM_SORT_PACKET* p = reinterpret_cast<CS_ITEM_SORT_PACKET*>(packet);
			std::vector<std::tuple<long long, short, short, bool>> slots;
			slots.reserve(p->item_count);
			for (int i = 0; i < p->item_count; ++i)
			{
				const SortSlot& slot = p->slots[i];
				slots.emplace_back(slot.item_uid, slot.x, slot.y, slot.is_rotated);
			}
			inventory_->ApplySortResult(slots);
			SaveInventoryToRedis();
			break;
		}
		// 아이템 버리기
		case CS_ITEM_DROP:
		{
			CS_ITEM_DROP_PACKET* p = reinterpret_cast<CS_ITEM_DROP_PACKET*>(packet);

			int map_id = GetNewMapItemId();
			if (map_id == -1) break;

			Item* droppedItem = inventory_->RemoveItem(p->item_uid);
			if (!droppedItem) break;  // 인벤에 없는 아이템

			MapItem* mapItem = dynamic_cast<MapItem*>(objects[map_id].get());
			if (!mapItem)
			{ 
				delete droppedItem; 
				break;
			}

			mapItem->id_ = map_id;
			mapItem->x_ = x_;
			mapItem->y_ = y_;
			mapItem->item_uid = droppedItem->item_uid;
			mapItem->template_id = droppedItem->template_id; 
			mapItem->state_ = OS_INGAME;
			mapItem->PutInSector();

			long long dropped_uid = droppedItem->item_uid;
			delete droppedItem;	// 원본 삭제

			// 주변 플레이어에게 알림 + Player의 view_list_에 MapItem 등록
			for (auto& sector : mapItem->around_sector_)
			{
				std::lock_guard<std::mutex> sec_l(g_ObjectSector[sector].mut_sector_);
				for (auto& pid : g_ObjectSector[sector].sec_id_)
				{
					if (IsPlayer(pid) && CanSee(mapItem->id_, pid))
					{
						// Player의 view_list_에 MapItem 추가
						objects[pid]->mut_view_.lock();
						objects[pid]->view_list_.insert(map_id);
						objects[pid]->mut_view_.unlock();
						// 패킷 전송
						mapItem->SendAddObjectPacket(pid);
					}
				}
			}

			SaveInventoryToRedis();
			g_db_request_queue.push({ DBRequest::DELETE_ITEM, id_, dropped_uid });
			break;
		}
		// 아이템 줍기
		case CS_ITEM_PICKUP:
		{
			MapItem* target = nullptr;
			std::chrono::system_clock::time_point latest_time;
			bool found = false;

			mut_view_.lock(); // view_list_ 안전하게 순회
			for (int view_id : view_list_) 
			{
				if (true == IsMapItem(view_id)) 
				{
					if (objects[view_id]->x_ == x_ && objects[view_id]->y_ == y_) 
					{
						MapItem* item = dynamic_cast<MapItem*>(objects[view_id].get());
						if (!item) continue;
						
						if (false == found || item->drop_time > latest_time)
						{
							target = item;
							latest_time = item->drop_time;
							found = true;
						}
					}
				}
			}
			mut_view_.unlock();

			if (target) 
			{
				// 다른 플레이어가 이미 주웠다면 OS_INGAME이 아니므로 실패
				{
					std::lock_guard<std::mutex> ll(objects[target->id_]->mut_state_);
					if (objects[target->id_]->state_ != OS_INGAME) break; // 이미 주워진 아이템
					objects[target->id_]->state_ = OS_FREE; // 선점
				}

				// 줍기 성공
				Item* newItem = new Item();
				newItem->item_uid = target->item_uid;
				newItem->template_id = target->template_id;
				// 인벤토리에 추가 (자동 빈칸)
				if (inventory_->AddItem(newItem)) 
				{
					int target_id = target->id_;

					// 인벤토리에 들어간 위치(x,y)가 확정된 직후 바로 전송
					SendGetItemPacket(newItem);
					
					// 주변에 제거 알림 (MapItem::SendRemoveObjectPacket이 view_list_도 정리함)
					for (auto& sector : target->around_sector_) 
					{
						std::lock_guard<std::mutex> sec_l(g_ObjectSector[sector].mut_sector_);
						for (auto& pid : g_ObjectSector[sector].sec_id_)
						{
							if (true == IsPlayer(pid)) 
							{
								if (true == CanSee(target_id, pid))
								{
									target->SendRemoveObjectPacket(pid);
								}
							}
						}
					}

					// 섹터에서 제거
					std::pair<int, int> sec = target->current_sector_;
					g_ObjectSector[sec].mut_sector_.lock();
					g_ObjectSector[sec].sec_id_.erase(target_id);
					g_ObjectSector[sec].mut_sector_.unlock();

					SaveInventoryToRedis();
					g_db_request_queue.push({ DBRequest::SAVE_ITEM, id_, newItem->item_uid });
				}
				else
				{
					// 인벤 꽉 참 - 선점한 state_를 OS_INGAME으로 복구
					{
						std::lock_guard<std::mutex> ll(objects[target->id_]->mut_state_);
						objects[target->id_]->state_ = OS_INGAME;
					}
					delete newItem;
				}
			}
			break;
		}
	}
}

// 플레이어 정보를 Redis에 저장 (실시간 동기화)
void Player::SaveToRedis()
{
	if (!g_redis_client->is_connected()) return;

	// Redis Key 생성
	std::string key(name_);
	key = "User:" + key;

	// vector of pair로 필드값 셋 설정
	std::vector<std::pair<std::string, std::string>> field_val = {
		{"x", std::to_string(x_)},
		{"y", std::to_string(y_)},
		{"hp", std::to_string(hp_)},
		{"level", std::to_string(level_)},
		{"exp", std::to_string(exp_)},
		{"OnlineFlag", "1"} // 1 = 현재 접속 중(서버 켜지는 중 상태로 복구)
	};

	// HSET 명령어로 저장 (비동기)
	g_redis_client->hmset(key, field_val, [](cpp_redis::reply& reply) {
		// if (reply.is_error()) std::cout << "Redis Set Error\n";
		});

	// 만료 시간은 7일(604800초)로 재설정 (접속할 때마다 갱신됨)
	g_redis_client->expire(key, 604800);

	// 파이프라인 커밋
	g_redis_client->commit();

	// 인벤토리도 같이 저장
	SaveInventoryToRedis();
}

void Player::SaveInventoryToRedis()
{
	if (!g_redis_client->is_connected() || !inventory_) return;

	std::string key = "UserInventory:" + std::string(name_);
	
	std::vector<std::pair<std::string, std::string>> field_val = inventory_->GetInventoryDataForRedis();

	if (!field_val.empty())
	{
		g_redis_client->hmset(key, field_val, [](cpp_redis::reply& reply) {});
		g_redis_client->expire(key, 604800);
		g_redis_client->commit();
	}
}

void Player::LoadInventoryFromRedis()
{
	if (!g_redis_client->is_connected() || !inventory_) return;

	std::string key = "UserInventory:" + std::string(name_);
	auto future_reply = g_redis_client->hgetall(key);
	g_redis_client->sync_commit();

	auto reply = future_reply.get();
	if (reply.is_array())
	{
		auto arr = reply.as_array();
		for (size_t i = 0; i < arr.size(); i += 2) 
		{
			std::string item_uid_str = arr[i].as_string();
			std::string val_str = arr[i + 1].as_string();

			// 파싱 "TID:X:Y:R"
			int tid, x, y, rot;
			if (sscanf_s(val_str.c_str(), "%d:%d:%d:%d", &tid, &x, &y, &rot) == 4)
			{
				Item* newItem = new Item();
				newItem->item_uid = std::stoll(item_uid_str);
				newItem->template_id = tid;
				newItem->x = (short)x;
				newItem->y = (short)y;
				newItem->is_rotated = (bool)rot;

				// PlaceItem 실패 시 즉시 해제 (누수 방지)
				if (!inventory_->PlaceItem(newItem, x, y, newItem->is_rotated))
				{
					std::cout << "[WARN] LoadInventoryFromRedis: PlaceItem failed"
						<< " uid=" << newItem->item_uid
						<< " pos=(" << x << "," << y << ")\n";
					delete newItem;
				}
			}
		}
	}
}

// Redis에서 데이터 삭제 (로그아웃 후 SQL 저장 완료 시)
void Player::DeleteFromRedis()
{
	if (!g_redis_client->is_connected()) return;
	std::string key(name_);
	key = "User:" + key;
	g_redis_client->del({ key });
	g_redis_client->commit();
}

void Player::DBSaveInventory(SQLHDBC& hdbc)
{
	if (!inventory_) return;

	std::wstring w_user_id(name_, name_ + strlen(name_));
	std::vector<Item*> items = inventory_->GetAllItems();

	SQLHSTMT hstmt = AllocateStatement(hdbc);
	if (!hstmt) return;

	// 트랜잭션 시작
	// BEGIN ~ COMMIT 사이의 모든 쿼리를 하나의 단위로 묶음
	// 중간 실패 시 ROLLBACK → DELETE만 된 채로 남는 상황 방지
	SQLRETURN retcode = SQLExecDirect(hstmt, (SQLWCHAR*)L"BEGIN TRANSACTION", SQL_NTS);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		return;
	}
	SQLCloseCursor(hstmt);

	// DELETE
	retcode = SQLPrepare(hstmt,
		(SQLWCHAR*)L"DELETE FROM user_inventory_table WHERE user_id = ?", SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		SQLLEN userIdLen = w_user_id.size() * sizeof(WCHAR);
		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR,
			20, 0, (SQLWCHAR*)w_user_id.c_str(), 0, &userIdLen);

		retcode = SQLExecute(hstmt);
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
			SQLExecDirect(hstmt, (SQLWCHAR*)L"ROLLBACK TRANSACTION", SQL_NTS);
			SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
			return;
		}
	}
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

	// INSERT
	if (!items.empty())
	{
		retcode = SQLPrepare(hstmt,
			(SQLWCHAR*)L"INSERT INTO user_inventory_table "
			L"(user_id, item_uid, template_id, pos_x, pos_y, is_rotated) "
			L"VALUES (?, ?, ?, ?, ?, ?)", SQL_NTS);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
			SQLExecDirect(hstmt, (SQLWCHAR*)L"ROLLBACK TRANSACTION", SQL_NTS);
			SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
			return;
		}

		for (Item* item : items)
		{
			SQLBIGINT   uid = item->item_uid;
			SQLINTEGER  tid = item->template_id;
			SQLSMALLINT px = item->x;
			SQLSMALLINT py = item->y;
			SQLSMALLINT rot = item->is_rotated ? 1 : 0;
			SQLLEN      userIdLen = w_user_id.size() * sizeof(WCHAR);

			SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR,
				20, 0, (SQLWCHAR*)w_user_id.c_str(), 0, &userIdLen);
			SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT,
				0, 0, &uid, 0, NULL);
			SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER,
				0, 0, &tid, 0, NULL);
			SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_SMALLINT,
				0, 0, &px, 0, NULL);
			SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_SMALLINT,
				0, 0, &py, 0, NULL);
			SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_SMALLINT,
				0, 0, &rot, 0, NULL);

			retcode = SQLExecute(hstmt);
			if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
			{
				DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
				// ROLLBACK: 지금까지의 DELETE + INSERT 전부 취소
				SQLExecDirect(hstmt, (SQLWCHAR*)L"ROLLBACK TRANSACTION", SQL_NTS);
				SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
				return; // 핸들 재사용 방지
			}
			SQLCloseCursor(hstmt);
			SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		}
	}

	// COMMIT 
	// 여기까지 왔다면 DELETE + 전체 INSERT 모두 성공
	// COMMIT으로 DB에 확정 반영
	SQLExecDirect(hstmt, (SQLWCHAR*)L"COMMIT TRANSACTION", SQL_NTS);
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

	std::cout << "[" << name_ << "] Inventory saved ("
		<< items.size() << " items)" << std::endl;
}

void Player::DBLoadInventory(SQLHDBC& hdbc)
{
	if (!inventory_) return;

	SQLHSTMT hstmt = AllocateStatement(hdbc);
	if (!hstmt) return;

	std::wstring w_user_id(name_, name_ + strlen(name_));
	SQLLEN userIdLen = SQL_NTS;

	SQLRETURN retcode = SQLPrepare(hstmt,
		(SQLWCHAR*)L"{CALL sp_LoadInventory(?)}", SQL_NTS);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		return;
	}

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR,
		20, 0, (SQLWCHAR*)w_user_id.c_str(), 0, &userIdLen);

	retcode = SQLExecute(hstmt);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		return;
	}

	SQLBIGINT   d_uid;
	SQLINTEGER  d_tid;
	SQLSMALLINT d_x, d_y, d_rot;
	SQLLEN cb_uid, cb_tid, cb_x, cb_y, cb_rot;

	SQLBindCol(hstmt, 1, SQL_C_SBIGINT, &d_uid, 0, &cb_uid);
	SQLBindCol(hstmt, 2, SQL_C_SLONG, &d_tid, 0, &cb_tid);
	SQLBindCol(hstmt, 3, SQL_C_SSHORT, &d_x, 0, &cb_x);
	SQLBindCol(hstmt, 4, SQL_C_SSHORT, &d_y, 0, &cb_y);
	SQLBindCol(hstmt, 5, SQL_C_SSHORT, &d_rot, 0, &cb_rot);

	int loaded = 0;
	while (SQLFetch(hstmt) == SQL_SUCCESS)
	{
		Item* newItem = new Item();
		newItem->item_uid = d_uid;
		newItem->template_id = (int)d_tid;
		newItem->x = d_x;
		newItem->y = d_y;
		newItem->is_rotated = (d_rot != 0);

		// PlaceItem 실패 시 메모리 해제 (누수 방지)
		if (!inventory_->PlaceItem(newItem, d_x, d_y, newItem->is_rotated))
		{
			std::cout << "[WARN] DBLoadInventory: PlaceItem failed uid="
				<< newItem->item_uid << " pos=(" << d_x << "," << d_y << ")\n";
			delete newItem;
			continue;
		}
		loaded++;
	}

	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	if (loaded > 0) std::cout << "[" << name_ << "] Inventory loaded from SQL (" << loaded << " items)\n";
}

bool Player::DBSaveItem(SQLHDBC& hdbc, long long item_uid)
{
	if (!inventory_) return false;

	Item* target = inventory_->FindItem(item_uid);
	if (!target) return false;

	SQLHSTMT hstmt = AllocateStatement(hdbc);
	if (!hstmt) return false;

	std::wstring w_user_id(name_, name_ + strlen(name_));
	SQLLEN userIdLen = SQL_NTS;

	SQLRETURN retcode = SQLPrepare(hstmt,
		(SQLWCHAR*)L"{CALL sp_SaveInventory(?, ?, ?, ?, ?, ?)}", SQL_NTS);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		return false;
	}

	SQLBIGINT   uid = target->item_uid;
	SQLINTEGER  tid = target->template_id;
	SQLSMALLINT px = target->x;
	SQLSMALLINT py = target->y;
	SQLSMALLINT rot = target->is_rotated ? 1 : 0;

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR,
		20, 0, (SQLWCHAR*)w_user_id.c_str(), 0, &userIdLen);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &uid, 0, NULL);
	SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &tid, 0, NULL);
	SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_SMALLINT, 0, 0, &px, 0, NULL);
	SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_SMALLINT, 0, 0, &py, 0, NULL);
	SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_SMALLINT, 0, 0, &rot, 0, NULL);

	retcode = SQLExecute(hstmt);
	bool success = (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO);
	if (!success)
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);

	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	return success;
}

bool Player::DBDeleteItem(SQLHDBC& hdbc, long long item_uid)
{
	SQLHSTMT hstmt = AllocateStatement(hdbc);
	if (!hstmt) return false;

	std::wstring w_user_id(name_, name_ + strlen(name_));
	SQLLEN userIdLen = SQL_NTS;

	SQLRETURN retcode = SQLPrepare(hstmt,
		(SQLWCHAR*)L"{CALL sp_DeleteItem(?, ?)}", SQL_NTS);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		return false;
	}

	SQLBIGINT uid = item_uid;

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR,
		20, 0, (SQLWCHAR*)w_user_id.c_str(), 0, &userIdLen);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT,
		0, 0, &uid, 0, NULL);

	retcode = SQLExecute(hstmt);
	bool success = (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO);
	if (!success)
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);

	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	return success;
}
